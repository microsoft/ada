using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI;
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
    public sealed partial class Controller : UserControl
    {
        public event EventHandler<String> CommandSelected;
        Popup popup;
        ColorPickerPanel picker;

        public Controller()
        {
            this.InitializeComponent();
        }

        public void HidePopup()
        {
            if (this.popup != null)
            {
                popup.IsOpen = false;
            }
        }


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
            }
            return popup;
        }

        private void OnCancelColor(object sender, EventArgs e)
        {
            HidePopup();
        }

        private void OnAcceptColor(object sender, EventArgs e)
        {
            string label = (string)popup.Tag;
            Color c = picker.Color;
            string command = null;
            if (label == "SetColor")
            {
                command = string.Format("/color/{0},{1},{2}", c.R, c.G, c.B);
            }
            else if (label == "SetDmxColor")
            {
                command = string.Format("/dmx/{0},{1},{2}", c.R, c.G, c.B);
            }
            if (!string.IsNullOrEmpty(command))
            {
                CommandSelected?.Invoke(this, command);
            }
            popup.IsOpen = false;
        }

        private void OnButtonClick(object sender, RoutedEventArgs e)
        {
            HidePopup();
            if (sender is Button button)
            {
                var msg = button.Tag as string;
                CommandSelected?.Invoke(button, msg);
            }
        }

        private void OnSetDmxColor(object sender, RoutedEventArgs e)
        {
            Button button = (Button)sender;
            var popup = GetOrCreateColorPopup(button);
            if (popup.IsOpen && popup.Tag is string s && s == "SetDmxColor")
            {
                // make it a toggle.
                popup.IsOpen = false;
                return;
            }
            popup.VerticalOffset = (this.ActualHeight - picker.Height) / 2;
            popup.HorizontalOffset = (this.ActualWidth - picker.Width) / 2;

            popup.Tag = "SetDmxColor";
            popup.IsOpen = false;
            popup.IsOpen = true;
        }

        private void OnSetColor(object sender, RoutedEventArgs e)
        {
            Button button = (Button)sender;
            var popup = GetOrCreateColorPopup(button);
            if (popup.IsOpen && popup.Tag is string s && s == "SetColor")
            {
                // make it a toggle.
                popup.IsOpen = false;
                return;
            }
            popup.VerticalOffset = (this.ActualHeight - picker.Height) / 2;
            popup.HorizontalOffset = (this.ActualWidth - picker.Width) / 2;

            popup.Tag = "SetColor";
            popup.IsOpen = false;
            popup.IsOpen = true;
        }
    }
}
