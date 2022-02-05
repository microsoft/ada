// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using AdaKiosk.Utilities;
using AdaSimulation;
using Azure.Messaging.WebPubSub;
using System;
using System.Diagnostics;
using System.Net.WebSockets;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Media;
using Websocket.Client;

namespace AdaKiosk.Controls
{
    /// <summary>
    /// Interaction logic for DebugStripPanel.xaml
    /// </summary>
    public partial class DebugStripPanel : UserControl
    {
        public event EventHandler<String> CommandSelected;

        public DebugStripPanel()
        {
            InitializeComponent();
            this.IsVisibleChanged += DebugStripPanel_IsVisibleChanged;
        }


        public void Show()
        {
            try
            {
                
            }
            catch (Exception ex)
            {
                OnShowError(this, ex);
            }
        }

        class AckMessage
        {
            public string type { get; set; }
            public int ackId { get; set; }
            public bool success { get; set; }
        }

        private void OnShowError(object sender, Exception e)
        {
            UiDispatcher.Instance.RunOnUIThread(() => { 
                TextBoxError.Text = e.Message + "\n\n" + e.StackTrace;
            });
        }

        private void DebugStripPanel_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (e.NewValue is bool b && !b)
            {
                HidePopup();
            }
        }

        private void OnManualGo(object sender, RoutedEventArgs e)
        {
            if (string.Compare(TextBoxColor.Text, "shutdown", StringComparison.OrdinalIgnoreCase) == 0)
            {
                Application.Current.Shutdown(0);
                return;
            }

            int pi = 0;
            int strip = 0;
            if (!GetValidInteger(TextBoxPi, PiError, out pi) ||
                !GetValidInteger(TextBoxStrip, StripError, out strip)) {
                return;
            }

            Color c = Colors.White;
            if (string.IsNullOrEmpty(TextBoxColor.Text))
            {
                TextBoxColor.Text = "white";
            }
            try
            {
                c = (Color)ColorConverter.ConvertFromString(TextBoxColor.Text);
                ColorError.Text = "";
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

        private void OnTextBoxFocus(object sender, RoutedEventArgs e)
        {
            if (sender is TextBox box)
            {
                box.SelectAll();
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
            popup.Tag = "SetColor";
            popup.IsOpen = false;
            popup.IsOpen = true;
            picker.Focus();
        }

        Popup popup;
        ColorPickerPanel picker;

        Popup GetOrCreateColorPopup(UIElement target)
        {
            if (popup == null)
            {
                popup = new Popup();
                popup.Placement = PlacementMode.Bottom;
                picker = new ColorPickerPanel()
                {
                    Background = Brushes.Black,
                    BorderBrush = Brushes.White,
                    BorderThickness = new Thickness(1),
                    Focusable = true,
                    Width = 600,
                    Height = 600,
                    SupportTransparency = false
                };
                popup.Child = picker;
                picker.Accept += OnPickColor;
                picker.Cancel += OnCancelColor;
            }
            popup.PlacementTarget = target;
            return popup;
        }

        private void OnCancelColor(object sender, EventArgs e)
        {
            HidePopup();
        }

        private void OnPickColor(object sender, EventArgs e)
        {
            string label = (string)popup.Tag;
            Color c = picker.Color;
            TextBoxColor.Text = c.ToString();
            popup.IsOpen = false;
        }

        public void HidePopup()
        {
            if (this.popup != null)
            {
                popup.IsOpen = false;
            }
        }

        private void OnColorKeyDwn(object sender, System.Windows.Input.KeyEventArgs e)
        {
        }

        private void OnShutdown(object sender, RoutedEventArgs e)
        {
            Application.Current.Shutdown(0);
        }
    }
}
