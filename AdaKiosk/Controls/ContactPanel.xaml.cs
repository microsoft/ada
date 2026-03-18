// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using AdaKiosk.Utilities;
using AdaSimulation;
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
        internal string ServerAddress;

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
            UiDispatcher.Instance.RunOnUIThread(this.Show);
        }

        public void Show()
        {
            if (TextBlockContact != null)
            {
                TextBlockContact.Text = "clovett@microsoft.com ";
                TextBlockIssues.Text = "https://github.com/microsoft/ada";
                TextBlockAddress.Text = observer?.IpAddress;
                TextBlockServerIp.Text = this.ServerAddress;

                var assembly = this.GetType().Assembly;
                this.TextBlockVersion.Text = assembly.GetName().Version.ToString();
                this.Visibility = System.Windows.Visibility.Visible;
            }
        }
    }
}
