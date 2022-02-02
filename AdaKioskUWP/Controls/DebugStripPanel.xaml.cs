using AdaKioskUWP.Utilities;
using Azure.Messaging.WebPubSub;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Text;
using Websocket.Client;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI;
using Windows.UI.ViewManagement;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

// The User Control item template is documented at https://go.microsoft.com/fwlink/?LinkId=234236

namespace AdaKioskUWP.Controls
{
    public sealed partial class DebugStripPanel : UserControl
    {
        public event EventHandler<String> CommandSelected;

        public DebugStripPanel()
        {
            this.InitializeComponent();
            var assembly = this.GetType().Assembly;
            this.TextBoxVersion.Text = assembly.GetName().Version.ToString();
        }

        public async void Show()
        {
            try
            {
                var connectionString = "Endpoint=https://adapubsubservice.webpubsub.azure.com;AccessKey=fiL87FUtQK2+ri4JtGZDpVt0Hvj66DjKI1seriz9CZc=;Version=1.0;";
                var serviceClient = new WebPubSubServiceClient(connectionString, "AdaKiosk");
                var url = serviceClient.GetClientAccessUri();

                using (var client = new WebsocketClient(url))
                {
                    // Disable the auto disconnect and reconnect because the sample would like the client to stay online even no data comes in
                    client.ReconnectTimeout = null;
                    client.MessageReceived.Subscribe(msg => HandleMessage(client, msg));
                    await client.Start();
                    Debug.WriteLine("WebSocket Connected.");
                }
            } 
            catch (Exception ex)
            {
                OnShowError(this, ex);
            }
        }

        private void HandleMessage(WebsocketClient client, ResponseMessage msg)
        {
            Debug.WriteLine($"Message received: {msg}");
            client.Send(Encoding.UTF8.GetBytes("Ada got your message: " + msg.Text));
        }

        public void HidePopup()
        {
            if (this.popup != null)
            {
                popup.IsOpen = false;
            }
        }
        private void OnAcceptColor(object sender, EventArgs e)
        {
            if (picker != null)
            {
                TextBoxColor.Text = picker.Color.ToString();
            }
            HidePopup();
        }

        private void OnPickColor(object sender, RoutedEventArgs e)
        {
            Button button = (Button)sender;
            var popup = GetOrCreateColorPopup(button);
            if (popup.IsOpen)
            {
                // make it a toggle.
                popup.IsOpen = false;
                return;
            }

            popup.VerticalOffset = (this.ActualHeight - picker.Height) / 2;
            popup.HorizontalOffset = (this.ActualWidth - picker.Width) / 2;

            var c = ColorParser.TryParseColor(TextBoxColor.Text);
            if (c.HasValue) picker.Color = c.Value;
            popup.Tag = "SetColor";
            popup.IsOpen = false;
            popup.IsOpen = true;

            //picker.Focus();
        }

        private void OnCancelColor(object sender, EventArgs e)
        {
            HidePopup();
        }

        Popup popup;
        ColorPickerPanel picker;
        SolidColorBrush black = new SolidColorBrush(Windows.UI.Color.FromArgb(255, 0, 0, 0));
        SolidColorBrush white = new SolidColorBrush(Windows.UI.Color.FromArgb(255, 255, 255, 255));

        Popup GetOrCreateColorPopup(UIElement target)
        {
            if (popup == null)
            {
                popup = new Popup();
                picker = new ColorPickerPanel() { Width = 600, Height = 600 };
                popup.Child = picker;
                picker.Accept += OnAcceptColor;
                picker.Cancel += OnCancelColor;
                picker.Error += OnShowError;
            }
            return popup;
        }

        private void OnShowError(object sender, Exception e)
        {
            this.Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, new Windows.UI.Core.DispatchedHandler(() =>
            {
                TextBoxError.Text = e.Message + "\n\n" + e.StackTrace;
            }));
        }

        private void OnTextBoxFocus(object sender, RoutedEventArgs e)
        {
            if (sender is TextBox box)
            {
                box.SelectAll();
                InputPane.GetForCurrentView().TryShow();
            }
        }

        private void OnColorKeyDwn(object sender, KeyRoutedEventArgs e)
        {
            if (e.Key == Windows.System.VirtualKey.Enter && string.Compare(TextBoxColor.Text, "shutdown", StringComparison.OrdinalIgnoreCase) == 0)
            {
                Application.Current.Exit();
                return;
            }
        }

        bool GetValidInteger(TextBox box, TextBlock error, out int result)
        {
            string s = box.Text;
            int i = 0;
            if (!int.TryParse(s, out i))
            {
                error.Text = "invalid integer";
                result = 0;
                return false;
            }
            else
            {
                error.Text = "";
                result = i;
            }
            return true;
        }

        private void OnManualGo(object sender, RoutedEventArgs e)
        {
            if (string.Compare(TextBoxColor.Text, "shutdown", StringComparison.OrdinalIgnoreCase) == 0)
            {
                Application.Current.Exit();
                return;
            }

            int pi = 0;
            int strip = 0;
            if (!GetValidInteger(TextBoxPi, PiError, out pi) ||
                !GetValidInteger(TextBoxStrip, StripError, out strip))
            {
                return;
            }

            Color c = Colors.White;
            if (string.IsNullOrEmpty(TextBoxColor.Text))
            {
                TextBoxColor.Text = "white";
            }
            try
            {
                ColorError.Text = "";
                var pc = ColorParser.TryParseColor(TextBoxColor.Text);
                if (pc.HasValue)
                {
                    c = pc.Value;
                }
                else
                {
                    ColorError.Text = "invalid color";
                }
            }
            catch (Exception ex)
            {
                ColorError.Text = ex.Message;
            }
            TextBoxColor.Text = c.ToString();

            string ledranges = TextBoxLed.Text;
            if (string.IsNullOrEmpty(ledranges) || ledranges == "all")
            {
                // set strip command.
                TextBoxLed.Text = "all";
                CommandSelected?.Invoke(this, $"/strip/adapi{pi + 1}/{strip}/{c.R},{c.G},{c.B}");
            }
            else
            {
                // set specified pixel ranges.
                string cmd = $"/pixels/{pi}/{strip}/{ledranges}/{c.R},{c.G},{c.B}";
                CommandSelected?.Invoke(this, cmd);
            }
        }
    }
}
