using AdaKiosk.Utilities;
using Azure.Messaging.WebPubSub;
using System;
using System.Net.WebSockets;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;
using Websocket.Client;

namespace AdaKioskUnitTest
{
    class Program
    {
        WebPubSubGroup client;

        static async Task Main(string[] args)
        {
            Program p = new Program();
            await Task.Run(p.SocketTest);
            Console.WriteLine("Press ENTER to exit...");
            Console.ReadLine();
        }

        async void SocketTest()
        {
            var connectionString = Environment.GetEnvironmentVariable("ADA_WEBPUBSUB_CONNECTION_STRING");
            client = new WebPubSubGroup();
            client.MessageReceived += Client_MessageReceived;
            await client.Connect(connectionString, "AdaKiosk", "unittest", "demogroup");
        }

        private void Client_MessageReceived(object sender, string msg)
        {
            Console.WriteLine(msg);
        }
    }

}
