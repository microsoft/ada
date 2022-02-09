using AdaSimulation;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

namespace AdaKiosk.Utilities
{
    class NetworkObserver
    {
        DelayedActions actions = new DelayedActions();
        bool closed;
        int pingDelaySeconds = 60;
        int monitorDelaySeconds = 60 * 15;

        public event EventHandler NetworkStatusChanged;

        public bool NetworkAvailable { get; set; }

        public string IpAddress { get; set; }

        public void Start()
        {
            actions.StartDelayedAction("ping", CheckIpAddress, TimeSpan.FromMilliseconds(0));
        }

        public void Stop()
        {
            closed = true;
            actions.CancelDelayedAction("ping");
        }

        async void CheckIpAddress()
        {
            int delay = pingDelaySeconds;
            try
            {
                if (closed) return;
                if (!System.Net.NetworkInformation.NetworkInterface.GetIsNetworkAvailable())
                {
                    actions.StartDelayedAction("connect", Start, TimeSpan.FromSeconds(pingDelaySeconds));
                    NetworkAvailable = false;
                    IpAddress = null;
                    actions.StartDelayedAction("notify", FireEvent, TimeSpan.FromMilliseconds(0));
                    return;
                }

                // this is a way to find a good local ip address that can reach the internet.
                using var socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
                var targets = Dns.GetHostAddresses("www.bing.com");
                await socket.ConnectAsync(new IPEndPoint(targets[0], 80));
                var localEndPoint = socket.LocalEndPoint as IPEndPoint;
                if (localEndPoint != null)
                {
                    var ip = localEndPoint.Address.ToString();
                    if (ip != IpAddress) {
                        IpAddress = ip;
                        NetworkAvailable = true;
                        actions.StartDelayedAction("notify", FireEvent, TimeSpan.FromMilliseconds(0));
                    }
                    // now that we have a network we can check less frequently.
                    delay = monitorDelaySeconds;
                }
                socket.Close();
            }
            catch (Exception)
            {
                if (IpAddress != null)
                {
                    IpAddress = null;
                    NetworkAvailable = false;
                    actions.StartDelayedAction("notify", FireEvent, TimeSpan.FromMilliseconds(0));
                }                
            }

            actions.StartDelayedAction("connect", Start, TimeSpan.FromSeconds(pingDelaySeconds));
        }

        void FireEvent()
        {
            if (NetworkStatusChanged != null)
            {
                NetworkStatusChanged(this, EventArgs.Empty);
            }
        }
    }
}
