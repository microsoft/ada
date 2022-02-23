// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Threading;
using System.Threading.Tasks;
using AdaServerRelay;
using Microsoft.AspNetCore.Http;
using Microsoft.AspNetCore.Mvc;
using Microsoft.Azure.WebJobs;
using Microsoft.Azure.WebJobs.Extensions.Http;
using Microsoft.Azure.WebJobs.Extensions.OpenApi.Core.Attributes;
using Microsoft.Azure.WebJobs.Extensions.OpenApi.Core.Enums;
using Microsoft.Extensions.Logging;
using Microsoft.OpenApi.Models;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace Ada.Function
{
    public class HttpServerRelay
    {
        const string user = "gateway"; // for now we only have one user!

        private readonly IWebPubSubGroup _pubSubService;

        public HttpServerRelay(IWebPubSubGroup pubSubService)
        {
            this._pubSubService = pubSubService;
        }


        [FunctionName("server")]
        [OpenApiOperation(operationId: "Run", tags: new[] { "name" })]
        [OpenApiSecurity("function_key", SecuritySchemeType.ApiKey, Name = "code", In = OpenApiSecurityLocationType.Query)]
        [OpenApiResponseWithBody(statusCode: HttpStatusCode.OK, contentType: "text/plain", bodyType: typeof(string), Description = "The OK response")]
        public async Task<HttpResponseMessage> Run(
            [HttpTrigger(AuthorizationLevel.Anonymous, "get", "post", Route = "{hub}/{group}")] HttpRequest req,
            string hub, string group,
            ILogger log)
        {
            if (!string.IsNullOrEmpty(hub) && !string.IsNullOrEmpty(group))
            {
                string message = req.Query["message"];
                if (string.IsNullOrEmpty(message) && req.Method == "POST")
                {
                    message = await new StreamReader(req.Body).ReadToEndAsync();
                }
                if (!string.IsNullOrEmpty(message))
                {
                    return await Post(hub, group, message, log);
                }
            }

            // provide simple HTTP user interface.
            var response = new HttpResponseMessage(HttpStatusCode.OK);
            string html = LoadResource("AdaServerRelay.index.html");
            string url = string.Format("{0}://{1}{2}", req.Scheme, req.Host, req.Path);
            html = html.Replace("@ENDPOINT@", url);
            response.Content = new StringContent(html);
            response.Content.Headers.ContentType = new MediaTypeHeaderValue("text/html");
            return response;
        }

        private static string LoadResource(string name)
        {
            using (var s = typeof(HttpServerRelay).Assembly.GetManifestResourceStream(name))
            {
                using (var reader = new StreamReader(s))
                {
                    return reader.ReadToEnd();
                }
            }
        }

        string GetValidJson(string message)
        {
            try
            {
                var obj = JToken.Parse(message);
                return message;
            }
            catch (Exception) //some other exception
            {
                return '"' + message + '"';
            }
        }

        public async Task<HttpResponseMessage> Post(string hub, string group, string message, ILogger log)
        {
            if (!_pubSubService.IsConnected || _pubSubService.HubName != hub || _pubSubService.GroupName != group)
            {
                string connectionString = Environment.GetEnvironmentVariable("AdaWebPubSubConnectionString");
                await _pubSubService.Connect(connectionString, hub, user, group, TimeSpan.FromSeconds(10));
            }

            string responseText = null;
            if (this._pubSubService.IsConnected)
            {
                message = GetValidJson(message);
                var response = await this._pubSubService.SendAndWaitAsync(message, TimeSpan.FromSeconds(5));
                while (response is AckMessage)
                {
                    // wait for one more out of band response which is the real payload we are looking for
                    response = await this._pubSubService.ReceiveAsync(TimeSpan.FromSeconds(10));
                }
                if (response is GroupMessage gm)
                {
                    responseText = System.Text.Json.JsonSerializer.Serialize(gm);
                }
                else if (response == null)
                {
                    responseText = "\"timeout\"";
                }
            }
            else
            {
                responseText = "\"Hub not connected\"";
            }

            var httpResponse = new HttpResponseMessage(HttpStatusCode.OK);
            httpResponse.Content = new StringContent(responseText);
            httpResponse.Content.Headers.ContentType = new MediaTypeHeaderValue("text/plain");
            return httpResponse;
        }

    }
}
