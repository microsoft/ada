// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using AdaKiosk.Controls;
using AdaKiosk.Utilities;
using AdaSimulation;
using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Windows;
using System.Windows.Input;

namespace AdaKiosk
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private WebPubSubGroup bus;
        public static Window Instance;
        private NetworkObserver observer = new NetworkObserver();
        private DelayedActions actions = new DelayedActions();
        private int InteractiveSleepDelay = 600;
        private int InitialSleepDelay = 600;
        const string hubName = "AdaKiosk";
        const string userName = "kiosk";
        const string groupName = "demogroup";
        int sleepTick = 0;
        bool debugEnabled = true;
        int pingTick = 0;
        int pongTick = 0;
        bool offline = false;
        const int PingTimeout = 15; // minutes;
        const int PongTimeout = 30; // seconds.

        enum ViewType
        {
            Story,
            Simulation,
            Control,
            Contact,
            Debug
        }

        private ViewType currentView = ViewType.Story;

        public MainWindow()
        {
            Instance = this;
            InitializeComponent();
            this.Loaded += OnMainWindowLoaded;
            this.observer.NetworkStatusChanged += OnNetworkStatusChanged;
            this.ScreenSaver.Closed += OnScreenSaverClosed;
            this.sim.SimulatingCommand += OnSimulatingCommand;
            this.controller.CommandSelected += OnSendCommand;
            this.strips.CommandSelected += OnSendCommand;
            this.ContactPanel.Observer = observer;
            this.UpdateView();
            // Hide the DEBUG tab when running on the actual Kiosk with user name Ada.
            if (string.Compare(Environment.GetEnvironmentVariable("USERNAME"), "ADA", StringComparison.OrdinalIgnoreCase) == 0)
            {
                ButtonDebug.Visibility = Visibility.Collapsed;
                this.sim.Editable = false;
                this.debugEnabled = false;
            }
        }

        private async void OnSendCommand(object sender, string command)
        {
            try
            {
                if (bus != null) await bus.SendMessage("\"" + command + "\"");
            }
            catch(Exception ex)
            {
                if (this.debugEnabled)
                {
                    ShowStatus(ex.Message);
                }
            }
        }

        private void OnMainWindowLoaded(object sender, RoutedEventArgs e)
        {
            observer.Start();
        }

        private void OnNetworkStatusChanged(object sender, EventArgs e)
        {
            if (observer.NetworkAvailable)
            {
                if (this.bus == null)
                {
                    ConnectMessageBus();
                }
            }
            else if (this.bus != null)
            {
                // network has gone away, so shut down the message bus, we will 
                // reconnect it when the network comes back.
                this.bus.MessageReceived -= OnMessageReceived;
                this.bus.Close();
                this.bus = null;
                this.sim.Offline = true;
            }
        }

        async void ConnectMessageBus()
        {
            var connectionString = Environment.GetEnvironmentVariable("ADA_WEBPUBSUB_CONNECTION_STRING");
            if (!string.IsNullOrEmpty(connectionString))
            {
                try
                {
                    this.bus = new WebPubSubGroup();
                    await bus.Connect(connectionString, hubName, userName, groupName);
                    this.bus.MessageReceived += OnMessageReceived; 
                    SendVersionInfo();
                    this.actions.StartDelayedAction("ping", OnPing, TimeSpan.FromSeconds(1));
                } 
                catch (Exception)
                {
                    this.bus = null;
                    return;
                }
            }
            else
            {
                ShowStatus("Missing ADA_WEBPUBSUB_CONNECTION_STRING");
            }

            // if nothing happens clear the screen in 10 minutes.
            StartDelayedSleep(InitialSleepDelay);
        }

        void SetOffline(bool flag)
        {
            if (offline != flag)
            {
                offline = flag;
                ShowStatus(offline ? "Ada is not online!" : "Ada is back!");
            }
            this.sim.Offline = flag;
        }

        private void CheckPing() 
        {
            if (this.pingTick != 0 && this.pongTick == this.pingTick)
            {
                // didn't receive a response last time, so is Ada down?
                SetOffline(true);
                this.ButtonControl.Visibility = Visibility.Collapsed;
                if (this.currentView == ViewType.Control)
                {
                    OnBlog(this, null);
                }
            }
            else
            {
                SetOffline(false);
                this.ButtonControl.Visibility = Visibility.Visible; 
            }
        }

        private void OnPing()
        {
            if (this.bus == null) return;
            // sends a ping request, which should return the ada power state.
            this.pingTick = this.pongTick = Environment.TickCount;
            OnSendCommand(this, "ping");
            this.actions.StartDelayedAction("ping", OnPing, TimeSpan.FromMinutes(PingTimeout));
            this.actions.StartDelayedAction("pong", CheckPing, TimeSpan.FromSeconds(PongTimeout));
        }

        private void StartDelayedSleep(int seconds)
        {
            if (this.ScreenSaver.Visibility == Visibility.Visible)
            {
                this.ScreenSaver.Stop();
                UpdateView();
            }
            actions.StartDelayedAction("Sleep", OnSleep, TimeSpan.FromSeconds(seconds));
        }

        private void OnMessageReceived(object sender, string msg)
        {
            Debug.WriteLine(msg);
            try
            {
                var message = Message.FromJson(msg);
                actions.StartDelayedAction("HandleMessage", () => { HandleMessage(message); }, TimeSpan.FromMilliseconds(1));
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
            }
            if (this.debugEnabled)
            {
                ShowStatus(msg);
            }
        }

        private void SendVersionInfo()
        {
            if (this.bus != null)
            {
                var version = this.GetType().Assembly.GetName().Version.ToString();
                this.OnSendCommand(this, "/kiosk/version/" + version);
            }
        }

        private void UpdateState(string message)
        {
            this.pongTick = Environment.TickCount; 
            this.sim.Offline = false;
            this.sim.Powered = (message != "/state/off");
            this.ButtonControl.Visibility = Visibility.Visible;
        }

        private void HandleDebug(string message)
        {
            if (message == "/debug/true")
            {
                ButtonDebug.Visibility = Visibility.Visible;
                ShowStatus("");
            }
            else if (message == "/debug/false")
            {
                ButtonDebug.Visibility = Visibility.Collapsed;
                ShowStatus("");
            }
        }

        protected void HandleMessage(Message message)
        {
            // allow one Kiosk to send a zone selection to another.
            if (message.User != userName || message.Text.StartsWith("/zone"))
            {
                if (message.User == "server")
                {
                    SetOffline(false);
                }
                var simpleMessage = message.Text;
                if (!string.IsNullOrEmpty(simpleMessage))
                {
                    if (simpleMessage.StartsWith("/state"))
                    {
                        UpdateState(simpleMessage);
                    }
                    else if (simpleMessage.StartsWith("/debug"))
                    {
                        HandleDebug(simpleMessage);
                    }
                    else if (simpleMessage.StartsWith("/kiosk/version/?"))
                    {
                        SendVersionInfo();
                    }
                    else if (simpleMessage.StartsWith("/kiosk/shutdown"))
                    {
                        Application.Current.Shutdown(0);
                    }
                }

                this.sim.HandleMessage(message);
                this.controller.HandleMessage(message);
            }
        }

        protected override void OnClosing(CancelEventArgs e)
        {
            if (bus  != null)
            {
                bus.Close();
            }
            base.OnClosing(e);
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
                OnSendCommand(this, e.ToString());
            }
        }


        private void UpdateView()
        {
            this.webView.Visibility = Visibility.Collapsed;
            this.sim.Visibility = Visibility.Collapsed;
            this.controller.Visibility = Visibility.Collapsed;
            this.ContactPanel.Visibility = Visibility.Collapsed;
            this.strips.Visibility = Visibility.Collapsed;

            switch (this.currentView)
            {
                case ViewType.Story:
                    this.webView.Visibility = Visibility.Visible; ;
                    sim.Stop();
                    this.strips.HidePopup();
                    this.controller.HidePopup();
                    break;
                case ViewType.Simulation:
                    this.sim.Visibility = Visibility.Visible;
                    this.sim.Focus();
                    this.strips.HidePopup();
                    this.controller.HidePopup();
                    break;
                case ViewType.Control:
                    this.controller.Visibility = Visibility.Visible;
                    sim.Stop();
                    this.strips.HidePopup();
                    break;
                case ViewType.Contact:
                    this.ContactPanel.Visibility = Visibility.Visible;
                    sim.Stop();
                    this.strips.HidePopup();
                    this.controller.HidePopup();
                    this.ContactPanel.Show();
                    break;
                case ViewType.Debug:
                    this.strips.Visibility = Visibility.Visible; ;
                    sim.Stop();
                    this.controller.HidePopup();
                    break;
                default:
                    break;
            }
        }

        private void OnSimulation(object sender, RoutedEventArgs e)
        {
            this.currentView = ViewType.Simulation;
            UpdateView();
            ButtonBlog.IsChecked = false;
            ButtonControl.IsChecked = false;
            ButtonContact.IsChecked = false;
            ButtonDebug.IsChecked = false;
        }

        private void OnControl(object sender, RoutedEventArgs e)
        {
            this.currentView = ViewType.Control;
            UpdateView();
            ButtonBlog.IsChecked = false;
            ButtonSim.IsChecked = false;
            ButtonContact.IsChecked = false;
            ButtonDebug.IsChecked = false;
        }

        private void OnBlog(object sender, RoutedEventArgs e)
        {
            this.currentView = ViewType.Story;
            UpdateView();
            ButtonSim.IsChecked = false;
            ButtonControl.IsChecked = false;
            ButtonContact.IsChecked = false;
            ButtonDebug.IsChecked = false;
        }
        private void OnContact(object sender, RoutedEventArgs e)
        {
            this.currentView = ViewType.Contact;
            UpdateView();
            ButtonBlog.IsChecked = false;
            ButtonSim.IsChecked = false;
            ButtonControl.IsChecked = false;
            ButtonDebug.IsChecked = false;
            this.strips.Focus();
            this.strips.Show();
        }

        private void OnDebug(object sender, RoutedEventArgs e)
        {
            this.currentView = ViewType.Debug;
            UpdateView();
            ButtonBlog.IsChecked = false;
            ButtonSim.IsChecked = false;
            ButtonContact.IsChecked = false;
            ButtonControl.IsChecked = false;
            this.strips.Focus();
            this.strips.Show();
        }

        bool fullScreen = true;

        protected override void OnPreviewMouseMove(MouseEventArgs e)
        {
            // There is an odd mouse move that always happens when screen saver kicks in,
            // this TickCount stops it from causing screen saver to flash and turn off again.
            if (Environment.TickCount > this.sleepTick + 5000)
            {
                StartDelayedSleep(InteractiveSleepDelay);
            }
            base.OnPreviewMouseMove(e);
        }

        protected override void OnPreviewStylusDown(StylusDownEventArgs e)
        {
            // There is an odd mouse move that always happens when screen saver kicks in,
            // this TickCount stops it from causing screen saver to flash and turn off again.
            if (Environment.TickCount > this.sleepTick + 5)
            {
                StartDelayedSleep(InteractiveSleepDelay);
            }
            base.OnPreviewStylusDown(e);
        }

        protected override void OnPreviewTouchDown(TouchEventArgs e)
        {
            // There is an odd mouse move that always happens when screen saver kicks in,
            // this TickCount stops it from causing screen saver to flash and turn off again.
            if (Environment.TickCount > this.sleepTick + 5)
            {
                StartDelayedSleep(InteractiveSleepDelay);
            }
            base.OnPreviewTouchDown(e);
        }

        protected override void OnPreviewKeyDown(KeyEventArgs e)
        {
            StartDelayedSleep(InteractiveSleepDelay);
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

        void OnSleep()
        {
            // a black shield cannot animate over the top of webView!
            webView.Visibility = Visibility.Collapsed;
            this.strips.HidePopup();
            this.controller.HidePopup();
            this.sim.Stop();
            if (this.debugEnabled)
            {
                ShowStatus("Sleep");
            }
            this.sleepTick = Environment.TickCount;
            this.ScreenSaver.Start();
        }

        private void OnScreenSaverClosed(object sender, string arg)
        {
            if (this.debugEnabled)
            {
                ShowStatus(arg);
            }
            StartDelayedSleep(InteractiveSleepDelay);
        }

    }
}
