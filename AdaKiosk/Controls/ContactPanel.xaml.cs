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
        internal string InternalAddress;
        internal string ServerAddress;

        public ContactPanel()
        {
            InitializeComponent();
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
                TextBlockAddress.Text = "" + this.InternalAddress;
                TextBlockServerIp.Text = "" + this.ServerAddress;

                var assembly = this.GetType().Assembly;
                this.TextBlockVersion.Text = assembly.GetName().Version.ToString();
                this.Visibility = System.Windows.Visibility.Visible;
            }
        }
    }
}
