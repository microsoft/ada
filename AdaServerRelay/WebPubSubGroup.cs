// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using Azure.Messaging.WebPubSub;
using System;
using System.Diagnostics;
using System.Net.WebSockets;
using System.Text.Json;
using System.Threading.Tasks;
using Websocket.Client;

namespace AdaServerRelay
{
    class WebPubSubGroup
    {
        private WebsocketClient client;
        private int ackId = 1;
        TaskCompletionSource<Message> pending;
        private string hubName;
        private string connectionId;
        private string groupName;
        private bool groupJoined;
        private string userId;

        public int NextId { get => ackId; set => ackId = value; }

        public bool IsConnected { get; set; }

        public string GroupName { get => groupName; }

        public string HubName { get => hubName; }

        public WebPubSubGroup()
        {
            Debug.WriteLine("New WebPubSubGroup");
        }

        public async Task Connect(string connectionString, string hubName, string userId, string groupName, TimeSpan timeout)
        {
            await this.Close();
            this.hubName = hubName;
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
            var src = new TaskCompletionSource<Message>();
            this.pending = src;
            await client.Start();
            var response = await src.Task;
            if (response != null && response.Type == "system")
            {
                if (response.Event == "connected")
                {
                    this.connectionId = response.ConnectionId;
                }
            }
            Debug.WriteLine("WebSocket Connected.");
            await this.JoinGroup(groupName, timeout);
            this.IsConnected = true;
        }

        public event EventHandler<Message> MessageReceived;

        void HandleMessage(ResponseMessage msg)
        {
            Debug.WriteLine("Received: " + msg.Text);
            Message m = Message.FromJson(msg.Text);

            if (m == null || m.FromUserId == this.userId)
            {
                return;
            }

            if (MessageReceived != null)
            {
                MessageReceived(this, m);
            }

            if (this.pending != null)
            {
                this.pending.SetResult(m);
                this.pending = null;
            }
        }

        public async Task JoinGroup(string group, TimeSpan timeout)
        {
            string joinGroup = JsonSerializer.Serialize(new
            {
                type = "joinGroup",
                group = group,
                ackId = this.ackId++
            });

            var resp = await this.InternalSendAndWaitAsync(joinGroup, timeout);
            this.groupJoined = true;
            // check ack response.
            Debug.WriteLine("Joined group.");
        }

        public async Task LeaveGroup(string group, TimeSpan timeout)
        {
            string leaveGroup = JsonSerializer.Serialize(new
            {
                type = "leaveGroup",
                group = group,
                ackId = this.ackId++
            });

            var resp = await this.InternalSendAndWaitAsync(leaveGroup, timeout);
            this.groupJoined = false;
            // check ack response.
            Debug.WriteLine("Left group.");
        }

        public Task SendMessage(string json)
        {
            if (this.IsConnected)
            {                
                int ackId = this.ackId++;
                string groupMessage = "{\"type\": \"sendToGroup\", \"group\": \"" + groupName + "\", \"dataType\": \"json\", \"data\": " +
                    json + ", \"ackId\": " + ackId.ToString() + "}";
                try
                {
                    Console.WriteLine("Sending: " + groupMessage);
                    client.Send(groupMessage);
                } 
                catch (Exception ex)
                {
                    Debug.WriteLine("SendMessage: " + ex.Message);
                    IsConnected = false;
                }
            }
            return Task.CompletedTask;
        }

        public async Task Close()
        {
            if (this.groupJoined)
            {
                await this.LeaveGroup(this.groupName, TimeSpan.FromSeconds(10));
            }

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
            this.IsConnected = false;
        }
        
        public async Task<Message> ReceiveAsync(TimeSpan timeout)
        {
            var pending = new TaskCompletionSource<Message>();
            this.pending = pending;
            var tasks = new Task[1];
            tasks[0] = this.pending.Task;
            return await Task.Run(() =>
            {
                int index = Task.WaitAny(tasks, timeout);
                if (index == 0)
                {
                    return pending.Task.Result;
                }
                return null;
            });
        }

        private async Task<Message> InternalSendAndWaitAsync(string message, TimeSpan timeout)
        {
            var pending = new TaskCompletionSource<Message>();
            this.pending = pending;
            try
            {
                Console.WriteLine("Sending: " + message);
                client.Send(message);
            }
            catch (Exception ex)
            {
                Debug.WriteLine("SendMessage: " + ex.Message);
                IsConnected = false;
                return new Message { Type = "error", Data = "disconnected" };
            }
            var tasks = new Task[1];
            tasks[0] = pending.Task;
            return await Task.Run(() =>
            {
                int index = Task.WaitAny(tasks, timeout);
                if (index == 0)
                {
                    return pending.Task.Result;
                }
                return new Message { Type = "timeout" };
            });
        }

        public async Task<Message> SendAndWaitAsync(string json, TimeSpan timeout)
        {
            int ackId = this.ackId++;
            string groupMessage = "{\"type\": \"sendToGroup\", \"group\": \"" + groupName + "\", \"dataType\": \"json\", \"data\": " +
                json + ", \"ackId\": " + ackId.ToString() + "}";
            return await InternalSendAndWaitAsync(groupMessage, timeout);
        }
    }

}
