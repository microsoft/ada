// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using AdaSimulation;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace AdaKiosk.Controls
{
    /// <summary>
    /// Interaction logic for Controller.xaml
    /// </summary>
    public partial class Controller : UserControl
    {
        public event EventHandler<String> CommandSelected;
        Popup popup;
        ColorPickerPanel picker;

        public Controller()
        {
            InitializeComponent();
            this.IsVisibleChanged += Controller_IsVisibleChanged;
        }

        public void HidePopup()
        {
            if (this.popup != null)
            {
                popup.IsOpen = false;
            }
        }


        private void Controller_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (e.NewValue is bool b && !b)
            {
                HidePopup();
            }
        }


        private void OnButtonClick(object sender, RoutedEventArgs e)
        {
            if (sender is Button button)
            {
                var msg = button.Tag as string;
                CommandSelected?.Invoke(button, msg);
            }
        }

        Popup GetOrCreateColorPopup(UIElement target)
        {
            if (popup == null)
            {
                popup = new Popup();
                popup.Placement = PlacementMode.Bottom;
                picker = new ColorPickerPanel() { 
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
            popup.Tag = "SetColor";
            popup.IsOpen = false;
            popup.IsOpen = true;
            picker.Focus();
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
            popup.Tag = "SetDmxColor";
            popup.IsOpen = false;
            popup.IsOpen = true;
            picker.Focus();
        }


        private void OnCancelColor(object sender, EventArgs e)
        {
            HidePopup();
        }

        private void OnPickColor(object sender, EventArgs e)
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

        internal void Save()
        {
            throw new NotImplementedException();
        }
    }
}