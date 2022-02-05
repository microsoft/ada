using System;
using System.Net;
using System.Net.Sockets;
using System.Threading.Tasks;
using System.Windows.Controls;

namespace AdaKiosk.Controls
{
    /// <summary>
    /// Interaction logic for ContactPanel.xaml
    /// </summary>
    public partial class ContactPanel : UserControl
    {
        public ContactPanel()
        {
            InitializeComponent();
        }

        public void Show()
        {
            TextBlockContact.Text = "helloada@microsoft.com ";
            TextBlockIssues.Text = "https://github.com/microsoft/ada";

            var assembly = this.GetType().Assembly;
            this.TextBlockVersion.Text = assembly.GetName().Version.ToString();
            
            this.TextBlockUserName .Text = Environment.GetEnvironmentVariable("USERNAME");

            Task.Run(UpdateIpAddress);
        }

        async void UpdateIpAddress()
        {
            try
            {
                // this is a way to find a good local ip address that can reach the internet.
                using var socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
                var targets = Dns.GetHostAddresses("www.bing.com");
                await socket.ConnectAsync(new IPEndPoint(targets[0], 80));
                var localEndPoint = socket.LocalEndPoint  as IPEndPoint;
                if (localEndPoint != null)
                {
                    ShowAddress(localEndPoint.Address.ToString());
                }
                socket.Close();
            }
            catch (Exception ex)
            {
                ShowAddress(ex.Message);
            }
        }

        void ShowAddress(string addr)
        {
            Dispatcher.Invoke(() =>
            {
                TextBlockAddress.Text = addr;
            });
        }
    }
}
