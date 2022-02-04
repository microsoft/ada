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
            while (true)
            {
                Console.Write("Enter a command to send or 'x' to exit: ");
                string cmd = Console.ReadLine();
                if (cmd == "x")
                {
                    return;
                }
                await p.client.SendMessage("\"" + cmd + "\"");
            }
        }

        async void SocketTest()
        {
            var connectionString = Environment.GetEnvironmentVariable("ADA_WEBPUBSUB_CONNECTION_STRING");
            client = new WebPubSubGroup();
            client.MessageReceived += Client_MessageReceived;
            await client.Connect(connectionString, "AdaKiosk", "unittest", "demogroup");
            Console.WriteLine("Ada Web PubSub connected...");
        }

        private void Client_MessageReceived(object sender, string msg)
        {
            Console.WriteLine(msg);
            Console.WriteLine("Enter a command to send or 'x' to exit: ");
        }
    }

}
