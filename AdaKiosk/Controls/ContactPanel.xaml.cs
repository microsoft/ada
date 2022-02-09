// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using AdaKiosk.Utilities;
using System;
using System.Windows.Controls;

namespace AdaKiosk.Controls
{
    /// <summary>
    /// Interaction logic for ContactPanel.xaml
    /// </summary>
    public partial class ContactPanel : UserControl
    {
        NetworkObserver observer;

        public ContactPanel()
        {
            InitializeComponent();
        }

        internal NetworkObserver Observer
        {
            get => observer;
            set
            {
                if (this.observer != null)
                {
                    observer.NetworkStatusChanged -= OnNetworkStatusChanged;
                }
                observer = value;
                if (observer != null)
                {
                    observer.NetworkStatusChanged += OnNetworkStatusChanged;
                }
            }
        }

        private void OnNetworkStatusChanged(object sender, EventArgs e)
        {
            TextBlockAddress.Text = observer?.IpAddress;
        }

        public void Show()
        {
            TextBlockContact.Text = "helloada@microsoft.com ";
            TextBlockIssues.Text = "https://github.com/microsoft/ada";

            var assembly = this.GetType().Assembly;
            this.TextBlockVersion.Text = assembly.GetName().Version.ToString();            
            this.TextBlockUserName.Text = Environment.GetEnvironmentVariable("USERNAME");

        }
    }
}
