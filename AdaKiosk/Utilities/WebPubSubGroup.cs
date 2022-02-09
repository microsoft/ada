// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using Azure.Messaging.WebPubSub;
using System;
using System.Diagnostics;
using System.Net.WebSockets;
using System.Text.Json;
using System.Threading.Tasks;
using Websocket.Client;

namespace AdaKiosk.Utilities
{
    class WebPubSubGroup
    {
        private WebsocketClient client;
        private int ackId = 1;
        TaskCompletionSource<ResponseMessage> pending;
        private string connectionId;
        private string groupName;
        private string userId;

        public async Task Connect(string connectionString, string hubName, string userId, string groupName)
        {
            this.groupName = groupName;
            this.userId = userId;
            var serviceClient = new WebPubSubServiceClient(connectionString, hubName);
            var url = serviceClient.GetClientAccessUri(userId: userId, roles: new string[] {
                    $"webpubsub.joinLeaveGroup.{groupName}", $"webpubsub.sendToGroup.{groupName}"
                });

            client = new WebsocketClient(url, () =>
            {
                var inner = new ClientWebSocket();
                inner.Options.AddSubProtocol("json.webpubsub.azure.v1");
                return inner;
            });

            // Disable the auto disconnect and reconnect because the sample would like the client to stay online even no data comes in
            client.ReconnectTimeout = null;
            client.MessageReceived.Subscribe(msg => {
                HandleMessage(msg);
            });
            var src = new TaskCompletionSource<ResponseMessage>();
            this.pending = src;
            await client.Start();
            var response = await src.Task;
            SystemMessage sys = JsonSerializer.Deserialize<SystemMessage>(response.Text);
            if (sys.@event == "connected")
            {
                this.connectionId = sys.connectionId;
            }
            Debug.WriteLine("WebSocket Connected.");
            await this.JoinGroup(groupName);
        }

        public event EventHandler<string> MessageReceived;

        void HandleMessage(ResponseMessage msg)
        {
            if (msg.Text.StartsWith("{\"type\":\"ack\""))
            {
                AckMessage sys = JsonSerializer.Deserialize<AckMessage>(msg.Text);
                if (sys.success)
                {
                    Debug.WriteLine("Previous op was a success!");
                    // todo: handle failures via pending.SetException?
                }
            }
            else if (msg.Text.StartsWith("{\"type\":\"message\""))
            {
                // received a message to the group!
                Debug.WriteLine("Group Message: " + msg.Text);
                if (MessageReceived != null)
                {
                    MessageReceived(this, msg.Text);
                }
            }

            Debug.WriteLine(msg.Text);
            if (this.pending != null)
            {
                this.pending.SetResult(msg);
                this.pending = null;
            }
        }

        private async Task JoinGroup(string group)
        {
            string joinGroup = JsonSerializer.Serialize(new
            {
                type = "joinGroup",
                group = group,
                ackId = this.ackId++
            });

            var resp = await this.SendAndWaitAsync(joinGroup);
            // check ack response.
            Debug.WriteLine("Joined group.");
        }

        public async Task SendMessage(string json)
        {
            int ackId = this.ackId++;
            string groupMessage = "{\"type\": \"sendToGroup\", \"group\": \"" + groupName + "\", \"dataType\": \"json\", \"data\": " +
                json + ", \"ackId\": " + ackId.ToString() + "}";
            var resp = await this.SendAndWaitAsync(groupMessage);
        }

        internal void Close()
        {
            if (this.pending != null)
            {
                this.pending.SetCanceled();
            }
            if (this.client != null)
            {
                try
                {
                    this.client.Dispose();
                }
                catch { }
            }
        }

        private async Task<ResponseMessage> SendAndWaitAsync(string message)
        {
            this.pending = new TaskCompletionSource<ResponseMessage>();
            client.Send(message);
            var response = await this.pending.Task;
            return response;
        }

        class AckMessage
        {
            public string type { get; set; }
            public int ackId { get; set; }
            public bool success { get; set; }
        }

        class SystemMessage
        {
            public string type { get; set; }
            public string @event { get; set; }
            public string userId { get; set; }
            public string connectionId { get; set; }
        }

        class GroupMessage
        {
            public string type { get; set; }
            public string group { get; set; }
            public string user { get; set; }
            public string dataType { get; set; }
            public string data { get; set; }
            public int ackId { get; set; }
        }
    }

}
