using AdaSimulation;
using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;

namespace AdaKiosk
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private MessageBus bus;
        public static Window Instance;
        private DelayedActions actions = new DelayedActions();

        public MainWindow()
        {
            Instance = this;
            InitializeComponent();
            this.Loaded += OnMainWindowLoaded;
            this.sim.SimulatingCommand += OnSimulatingCommand;
            this.controller.CommandSelected += OnSendCommand;
            this.strips.CommandSelected += OnSendCommand;
        }

        private async void OnSendCommand(object sender, string command)
        {
            try
            {
                if (bus != null) await bus.SendMessage("server", command);
            }
            catch(Exception ex)
            {
                ShowStatus(ex.Message);
            }
        }

        private async void OnMainWindowLoaded(object sender, RoutedEventArgs e)
        {
            bus = new MessageBus();
            if (!await bus.ConnectAsync("ada", "kiosk"))
            {
                ShowStatus("No hub context");
            }
            else
            {
                bus.MessageReceived += OnSignalReceived;
                bus.Reconnecting += OnReconnecting;
                bus.Reconnected += OnReconnected;
                bus.Closed += OnConnectionClosed;
                await bus.SendMessage("server", "/kiosk/started");
            }
        }

        private void OnSignalReceived(object sender, string msg)
        {
            Debug.WriteLine(msg);
            try
            {
                var message = Message.FromJson(msg);
                sim.HandleMessage(message);
            } 
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
            }
            ShowStatus(msg);
        }

        private void OnReconnecting(object sender, Exception arg)
        {
            ShowStatus("reconnecting");
        }

        private void OnReconnected(object sender, string arg)
        {
            ShowStatus("reconnected");
        }

        private void OnConnectionClosed(object sender, Exception arg)
        {
            ShowStatus("connection closed");
        }

        protected override void OnClosing(CancelEventArgs e)
        {
            using (bus)
            {
                bus = null;
            }
            base.OnClosing(e);
        }

        private void OnSignalReceived(string msg)
        {
            Debug.WriteLine(msg);
            ShowStatus(msg);
        }

        private void ShowStatus(string status)
        {
            UiDispatcher.Instance.RunOnUIThread(() =>
            {
                TextMessage.Text = status;
            });
        }

        private void OnSimulatingCommand(object sender, SimulatedCommand e)
        {
            if (bus != null)
            {
                _ = bus.SendMessage("server", e.ToString());
            }
        }

        private void OnSimulation(object sender, RoutedEventArgs e)
        {
            this.webView.Visibility = Visibility.Collapsed;
            this.sim.Visibility = Visibility.Visible;
            this.controller.Visibility = Visibility.Collapsed;
            this.strips.Visibility = Visibility.Collapsed;
            ButtonBlog.IsChecked = false;
            ButtonControl.IsChecked = false;
            ButtonDebug.IsChecked = false;
            this.sim.Focus();
        }

        private void OnControl(object sender, RoutedEventArgs e)
        {
            this.webView.Visibility = Visibility.Collapsed;
            this.sim.Visibility = Visibility.Collapsed;
            this.controller.Visibility = Visibility.Visible;
            this.strips.Visibility = Visibility.Collapsed;
            sim.Stop();
            ButtonBlog.IsChecked = false;
            ButtonSim.IsChecked = false;
            ButtonDebug.IsChecked = false;
        }

        private void OnBlog(object sender, RoutedEventArgs e)
        {
            this.webView.Visibility = Visibility.Visible;
            this.sim.Visibility = Visibility.Collapsed;
            this.controller.Visibility = Visibility.Collapsed;
            this.strips.Visibility = Visibility.Collapsed;
            sim.Stop();
            ButtonSim.IsChecked = false;
            ButtonControl.IsChecked = false;
            ButtonDebug.IsChecked = false;
        }

        private void OnDebug(object sender, RoutedEventArgs e)
        {
            this.webView.Visibility = Visibility.Collapsed;
            this.sim.Visibility = Visibility.Collapsed;
            this.controller.Visibility = Visibility.Collapsed;
            this.strips.Visibility = Visibility.Visible;
            sim.Stop();
            ButtonBlog.IsChecked = false;
            ButtonSim.IsChecked = false;
            ButtonControl.IsChecked = false;
            this.strips.Focus();
        }

        bool fullScreen = true;

        protected override void OnPreviewKeyDown(KeyEventArgs e)
        {
            if (e.Key == Key.F11)
            {
                if (!fullScreen)
                {
                    fullScreen = true;
                    this.WindowStyle = WindowStyle.None;
                    this.WindowState = WindowState.Maximized;
                }
                else
                {
                    fullScreen = false;
                    this.WindowStyle = WindowStyle.SingleBorderWindow;
                    this.WindowState = WindowState.Normal;
                }
                e.Handled = true;
            }
            else if (e.Key == Key.S)
            {
                if ((Keyboard.Modifiers & ModifierKeys.Control) != 0)
                {
                    Save();
                }
            }
            base.OnPreviewKeyDown(e);
        }

        private void OnSave(object sender, ExecutedRoutedEventArgs e)
        {
            Save();
        }

        void Save()
        { 
            if (this.controller.Visibility == Visibility.Visible)
            {
                this.controller.Save();
            }
            else if (this.sim.Visibility == Visibility.Visible)
            {
                this.sim.Save();
            }
            ShowStatus("saved");
        }
    }
}
