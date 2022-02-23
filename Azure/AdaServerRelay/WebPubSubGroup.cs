﻿// Copyright (c) Microsoft Corporation.
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

    public class BaseMessage
    {
        public string type { get; set; }
    }

    public class ErrorMessage : BaseMessage
    {
        public string reason { get; set; }
    }

    public class AckMessage : BaseMessage
    {
        public int ackId { get; set; }
        public bool success { get; set; }
    }

    public class SystemMessage : BaseMessage
    {
        public string @event { get; set; }
        public string userId { get; set; }
        public string connectionId { get; set; }
    }

    public class GroupMessage : BaseMessage
    {
        public string from { get; set; }
        public string fromUserId { get; set; }
        public string group { get; set; }
        public string dataType { get; set; }
        public string data { get; set; }
    }

    public interface IWebPubSubGroup
    {
        bool IsConnected { get; }
        public string GroupName { get; }
        public string HubName { get; }
        public int NextId { get; set; }
        Task Connect(string connectionString, string hubName, string userId, string groupName, TimeSpan timeout);
        Task SendMessage(string json);
        Task JoinGroup(string group, TimeSpan timeout);
        Task<BaseMessage> SendAndWaitAsync(string message, TimeSpan timeout);
        Task<BaseMessage> ReceiveAsync(TimeSpan timeout);
        event EventHandler<GroupMessage> MessageReceived;
    }

    class WebPubSubGroup : IWebPubSubGroup
    {
        private WebsocketClient client;
        private int ackId = 1;
        TaskCompletionSource<BaseMessage> pending;
        private string hubName;
        private string connectionId;
        private string groupName;
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
            this.Close();
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
            var src = new TaskCompletionSource<BaseMessage>();
            this.pending = src;
            await client.Start();
            var response = await src.Task;
            if (response is SystemMessage sys)
            {
                if (sys.@event == "connected")
                {
                    this.connectionId = sys.connectionId;
                }
            }
            Debug.WriteLine("WebSocket Connected.");
            await this.JoinGroup(groupName, timeout);
            this.IsConnected = true;
        }

        public event EventHandler<GroupMessage> MessageReceived;

        void HandleMessage(ResponseMessage msg)
        {
            Debug.WriteLine(msg.Text);
            BaseMessage bm = null;

            if (msg.Text.StartsWith("{\"type\":\"ack\""))
            {
                AckMessage sys = JsonSerializer.Deserialize<AckMessage>(msg.Text);
                if (sys.success)
                {
                    Debug.WriteLine("Previous op was a success!");
                    // todo: handle failures via pending.SetException?
                }
                bm = sys;
            }
            else if (msg.Text.StartsWith("{\"type\":\"message\""))
            {
                // received a message to the group!
                GroupMessage gm = JsonSerializer.Deserialize<GroupMessage>(msg.Text);
                if (gm.fromUserId == this.userId)
                {
                    // ignore messages to the group that we sent!
                    return;
                }
                Debug.WriteLine("Group Message: " + msg.Text);
                if (MessageReceived != null)
                {
                    MessageReceived(this, gm);
                }
                bm = gm;
            }
            else if (msg.Text.StartsWith("{\"type\":\"system\""))
            {
                bm = JsonSerializer.Deserialize<SystemMessage>(msg.Text);
            }
            else
            {
                Debug.WriteLine("???");
            }
            if (bm != null)
            {
                if (this.pending != null)
                {
                    this.pending.SetResult(bm);
                    this.pending = null;
                }
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
            // check ack response.
            Debug.WriteLine("Joined group.");
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
            this.IsConnected = false;
        }
        
        public async Task<BaseMessage> ReceiveAsync(TimeSpan timeout)
        {
            var pending = new TaskCompletionSource<BaseMessage>();
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

        private async Task<BaseMessage> InternalSendAndWaitAsync(string message, TimeSpan timeout)
        {
            var pending = new TaskCompletionSource<BaseMessage>();
            this.pending = pending;
            try
            {
                client.Send(message);
            }
            catch (Exception ex)
            {
                Debug.WriteLine("SendMessage: " + ex.Message);
                IsConnected = false;
                return new ErrorMessage { type = "disconnected" };
            }
            var tasks = new Task[1];
            tasks[0] = this.pending.Task;
            return await Task.Run(() =>
            {
                int index = Task.WaitAny(tasks, timeout);
                if (index == 0)
                {
                    return pending.Task.Result;
                }
                return new ErrorMessage { type = "timeout" };
            });
        }

        public async Task<BaseMessage> SendAndWaitAsync(string json, TimeSpan timeout)
        {
            int ackId = this.ackId++;
            string groupMessage = "{\"type\": \"sendToGroup\", \"group\": \"" + groupName + "\", \"dataType\": \"json\", \"data\": " +
                json + ", \"ackId\": " + ackId.ToString() + "}";
            return await InternalSendAndWaitAsync(groupMessage, timeout);
        }
    }

}
