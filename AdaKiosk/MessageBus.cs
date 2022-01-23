// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using Microsoft.AspNetCore.SignalR.Client;
using Microsoft.Azure.SignalR.Management;
using Microsoft.Extensions.Hosting.Internal;
using Microsoft.Extensions.Logging;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace AdaKiosk
{
    class MessageBus : IDisposable
    {
        ServiceHubContext hubContext;
        HubConnection connection;
        ServiceManager serviceManager;
        ApplicationLifetime lifetime;
        CancellationTokenSource tokenSource;
        string accessToken;
        string userName;

        public MessageBus()
        {
        }

        public async Task<bool> ConnectAsync(string hubName, string userName)
        {
            this.userName = userName;
            this.lifetime = new ApplicationLifetime(null);
            string connectionString = Environment.GetEnvironmentVariable("AZURE_SIGNALR_CONNECTION_STRING");
            if (!string.IsNullOrEmpty(connectionString))
            {
                var factory = new LoggerFactory();
                // factory.AddProvider(new Microsoft.Extensions.Logging.Debug.DebugLoggerProvider());
                serviceManager = new ServiceManagerBuilder().WithOptions(option =>
                {
                    option.ConnectionString = connectionString;
                    option.ServiceTransportType = ServiceTransportType.Persistent;
                }).WithLoggerFactory(factory)
                .BuildServiceManager();

                tokenSource = new CancellationTokenSource();
                hubContext = await serviceManager.CreateHubContextAsync(hubName, tokenSource.Token);
                var negotiationResponse = await hubContext.NegotiateAsync(new NegotiationOptions() { UserId = "server" });

                Uri uri = new Uri(negotiationResponse.Url);
                accessToken = negotiationResponse.AccessToken;
                var baseUri = uri.GetComponents(UriComponents.SchemeAndServer | UriComponents.Path, UriFormat.SafeUnescaped);

                connection = new HubConnectionBuilder().ConfigureLogging(logging =>
                {
                    logging.AddDebug();
                }).WithUrl(uri, (options) =>
                {
                    options.AccessTokenProvider = GetAccessToken;
                }).Build();

                connection.Closed += Connection_Closed;
                connection.Reconnected += Connection_Reconnected;
                connection.Reconnecting += Connection_Reconnecting;
                connection.On<string>("log", OnSignalReceived);

                await connection.StartAsync();
                return true;
            }
            return false;
        }

        public event EventHandler<string> MessageReceived;
        public event EventHandler<Exception> Reconnecting;
        public event EventHandler<string> Reconnected;
        public event EventHandler<Exception> Closed;

        private void OnSignalReceived(string msg)
        {
            MessageReceived?.Invoke(this, msg);
        }

        private async Task Connection_Reconnecting(Exception arg)
        {
            await Task.CompletedTask;
            Reconnecting?.Invoke(this, arg);
        }

        private async Task Connection_Reconnected(string arg)
        {
            await Task.CompletedTask;
            Reconnected?.Invoke(this, arg);
        }

        private async Task Connection_Closed(Exception arg)
        {
            await Task.CompletedTask;
            Closed?.Invoke(this, arg);
        }

        async Task<string> GetAccessToken()
        {
            await Task.CompletedTask;
            return this.accessToken;
        }

        protected virtual void Dispose(bool disposing)
        {
            if (connection != null)
            {
                this.lifetime.StopApplication();
                this.tokenSource.Cancel();
                _ = this.connection.StopAsync();

                using (serviceManager)
                {
                    serviceManager = null;
                }
                using (hubContext)
                {
                    hubContext = null;
                }
                this.connection = null;
            }
        }

        public async Task SendMessage(string toUser, string message)
        {
            if (this.hubContext != null)
            {
                var msg = new Message() { user = this.userName, message = message };
                var data = JsonConvert.SerializeObject(msg);
                var args = new object[] { data };
                var connection = this.hubContext.Clients.User(toUser);
                await connection.SendCoreAsync("log", args);
            }
        }

        // TODO: override finalizer only if 'Dispose(bool disposing)' has code to free unmanaged resources
        ~MessageBus()
        {
            // Do not change this code. Put cleanup code in 'Dispose(bool disposing)' method
            Dispose(disposing: false);
        }

        public void Dispose()
        {
            // Do not change this code. Put cleanup code in 'Dispose(bool disposing)' method
            Dispose(disposing: true);
            GC.SuppressFinalize(this);
        }

        class Message
        {
            public string user;
            public string message;
        }

    }
}
