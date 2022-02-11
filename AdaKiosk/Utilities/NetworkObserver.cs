using AdaSimulation;
using System;
using System.Collections.Generic;
using System.Diagnostics;
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
            bool hasInternet = false;

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
                foreach (var addr in from t in targets where t.AddressFamily == AddressFamily.InterNetwork select t) 
                {
                    await socket.ConnectAsync(new IPEndPoint(addr, 80));
                    try
                    {
                        var localEndPoint = socket.LocalEndPoint as IPEndPoint;
                        if (localEndPoint != null)
                        {
                            var ip = localEndPoint.Address.ToString();
                            if (ip != IpAddress)
                            {
                                hasInternet = true;
                                IpAddress = ip;
                                NetworkAvailable = true;
                                actions.StartDelayedAction("notify", FireEvent, TimeSpan.FromMilliseconds(0));
                            }
                        }
                        socket.Close();
                        break;
                    }
                    catch (Exception ex)
                    {
                        // try one of the other target addresses...
                        Debug.WriteLine(ex.Message);
                    }
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
            }

            if (!hasInternet)
            {
                if (IpAddress != null)
                {
                    IpAddress = null;
                    NetworkAvailable = false;
                    actions.StartDelayedAction("notify", FireEvent, TimeSpan.FromMilliseconds(0));
                }
                actions.StartDelayedAction("connect", Start, TimeSpan.FromSeconds(pingDelaySeconds));
            }
            else
            {
                // now that we have a network we can check less frequently.
                actions.StartDelayedAction("connect", Start, TimeSpan.FromSeconds(monitorDelaySeconds));
            }
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
