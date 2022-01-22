using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Http;
using Microsoft.AspNetCore.Mvc;
using Microsoft.Azure.SignalR.Management;
using Microsoft.Azure.WebJobs;
using Microsoft.Azure.WebJobs.Extensions.Http;
using Microsoft.Azure.WebJobs.Extensions.OpenApi.Core.Attributes;
using Microsoft.Azure.WebJobs.Extensions.OpenApi.Core.Enums;
using Microsoft.Extensions.Logging;
using Microsoft.OpenApi.Models;
using Newtonsoft.Json;

namespace Ada.Function
{
    public static class HttpServerRelay
    {
        class HubInfo
        {
            public string Hub;
            public string User;
            public DateTime LastAccessed;
            public IServiceHubContext Context;
        }

        static ConcurrentDictionary<string, HubInfo> s_cache = new ConcurrentDictionary<string, HubInfo>();
        const string user = "server"; // for now we only have one user!

        [FunctionName("server")]
        [OpenApiOperation(operationId: "Run", tags: new[] { "name" })]
        [OpenApiSecurity("function_key", SecuritySchemeType.ApiKey, Name = "code", In = OpenApiSecurityLocationType.Query)]
        [OpenApiParameter(name: "name", In = ParameterLocation.Query, Required = true, Type = typeof(string), Description = "The **Name** parameter")]
        [OpenApiResponseWithBody(statusCode: HttpStatusCode.OK, contentType: "text/plain", bodyType: typeof(string), Description = "The OK response")]
        public static async Task<HttpResponseMessage> Run(
            [HttpTrigger(AuthorizationLevel.Anonymous, "get", "post", Route = "{hub}/{command}")] HttpRequest req,
            string hub, string command,
            ILogger log)
        {
            if (command == "negotiate")
            {
                return await Negotiate(hub, user, log);
            }
            else if (command == "post")
            {
                string message = req.Query["message"];
                string requestBody = await new StreamReader(req.Body).ReadToEndAsync();
                message = message ?? requestBody;
                return await Post(hub, user, message, log);
            }

            var response = new HttpResponseMessage(HttpStatusCode.OK);
            string html = LoadResource("AdaServerRelay.index.html");
            html = html.Replace("@HUB@", hub);
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

        public static async Task<HttpResponseMessage> Negotiate(string hub, string user, ILogger log)
        {
            var factory = new LoggerFactory();
            factory.AddProvider(new Microsoft.Extensions.Logging.Debug.DebugLoggerProvider());
            string connectionString = Environment.GetEnvironmentVariable("AzureSignalRConnectionString");
            var serviceManager = new ServiceManagerBuilder().WithOptions(option =>
            {
                option.ConnectionString = connectionString;
                option.ServiceTransportType = ServiceTransportType.Persistent;
            }).WithLoggerFactory(factory)
             .BuildServiceManager();

            var tokenSource = new CancellationTokenSource();
            var hubContext = await serviceManager.CreateHubContextAsync(hub, tokenSource.Token);                        
            var negotiationResponse = await hubContext.NegotiateAsync(new NegotiationOptions() { UserId = "server", EnableDetailedErrors=true });
            
            var info = new HubInfo()
            {
                Hub = hub,
                User = user,
                Context = hubContext,
                LastAccessed = DateTime.Now
            };
            s_cache[hub] = info;
            CollectUnusedContexts(log);

            var tokens = new Dictionary<string, string>()
            {
                { "url", negotiationResponse.Url },
                { "accessToken", negotiationResponse.AccessToken }
            };

            var json = JsonConvert.SerializeObject(tokens);
            var response = new HttpResponseMessage(HttpStatusCode.OK);
            response.Content = new StringContent(json);
            response.Content.Headers.ContentType = new MediaTypeHeaderValue("text/html");
            return response;
        }

        public static async Task<HttpResponseMessage> Post(string hub, string user, string message, ILogger log)
        {
            HubInfo info;
            if (!s_cache.TryGetValue(hub, out info))
            {
                await Negotiate(hub, user, log);
            }

            if (s_cache.TryGetValue(hub, out info))
            {
                info.LastAccessed = DateTime.Now;
                var args = new object[] { message };
                await info.Context.Clients.User(user).SendCoreAsync("log", args);
            }

            CollectUnusedContexts(log);

            var response = new HttpResponseMessage(HttpStatusCode.OK);
            response.Content = new StringContent("post success");
            response.Content.Headers.ContentType = new MediaTypeHeaderValue("text/html");
            return response;
        }

        private static void CollectUnusedContexts(ILogger log)
        {
            // remove any contexts that have not been used in the last hour.
            List<string> copy = new List<string>(s_cache.Keys);
            foreach(var key in copy)
            {
                if (s_cache.TryGetValue(key, out HubInfo info))
                {
                    if (info.LastAccessed > DateTime.Now.AddHours(1))
                    {
                        s_cache.TryRemove(key, out info);
                        log.LogInformation($"Removing cached entry for {key}");
                    }
                }
            }
        }
    }
}
