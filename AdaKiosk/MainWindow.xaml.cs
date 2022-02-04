// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using AdaKiosk.Controls;
using AdaKiosk.Utilities;
using AdaSimulation;
using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Animation;

namespace AdaKiosk
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private WebPubSubGroup bus;
        public static Window Instance;
        private DelayedActions actions = new DelayedActions();
        private int InteractiveSleepDelay = 60;
        private int InitialSleepDelay = 600;
        const string userName = "kiosk";

        enum ViewType
        {
            Story,
            Simulation,
            Control,
            Debug
        }

        private ViewType currentView = ViewType.Story;

        public MainWindow()
        {
            Instance = this;
            InitializeComponent();
            this.Loaded += OnMainWindowLoaded;
            this.ScreenSaver.Closed += OnScreenSaverClosed;
            this.sim.SimulatingCommand += OnSimulatingCommand;
            this.controller.CommandSelected += OnSendCommand;
            this.strips.CommandSelected += OnSendCommand;
            this.sim.UserName = userName;
            this.UpdateView();
            // Hide the DEBUG tab when running on the actual Kiosk with user name Ada.
            //if (string.Compare(Environment.GetEnvironmentVariable("USERNAME"), "ADA", StringComparison.OrdinalIgnoreCase) == 0)
            //{
            //    ButtonDebug.Visibility = Visibility.Collapsed;
            //}
        }

        private async void OnSendCommand(object sender, string command)
        {
            try
            {
                if (bus != null) await bus.SendMessage("\"" + command + "\"");
            }
            catch(Exception ex)
            {
                ShowStatus(ex.Message);
            }
        }

        private async void OnMainWindowLoaded(object sender, RoutedEventArgs e)
        {
            var connectionString = Environment.GetEnvironmentVariable("ADA_WEBPUBSUB_CONNECTION_STRING");
            if (!string.IsNullOrEmpty(connectionString))
            {
                this.bus = new WebPubSubGroup();
                await bus.Connect(connectionString, "AdaKiosk", userName, "demogroup");
                this.bus.MessageReceived += OnMessageReceived;
                await bus.SendMessage("\"/kiosk/started\"");
            }
            else
            {
                ShowStatus("Missing ADA_WEBPUBSUB_CONNECTION_STRING");
            }

            // if nothing happens clear the screen in 10 minutes.
            StartDelayedSleep(InitialSleepDelay);
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
                sim.HandleMessage(message);
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
            }
            ShowStatus(msg);
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
                _ = bus.SendMessage("\"" + e.ToString() + "\"");
            }
        }


        private void UpdateView()
        {
            this.webView.Visibility = Visibility.Collapsed;
            this.sim.Visibility = Visibility.Collapsed;
            this.controller.Visibility = Visibility.Collapsed;
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
                    this.controller.Visibility = Visibility.Visible; ;
                    sim.Stop();
                    this.strips.HidePopup();
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
            ButtonDebug.IsChecked = false;
        }

        private void OnControl(object sender, RoutedEventArgs e)
        {
            this.currentView = ViewType.Control;
            UpdateView();
            ButtonBlog.IsChecked = false;
            ButtonSim.IsChecked = false;
            ButtonDebug.IsChecked = false;
        }

        private void OnBlog(object sender, RoutedEventArgs e)
        {
            this.currentView = ViewType.Story;
            UpdateView();
            ButtonSim.IsChecked = false;
            ButtonControl.IsChecked = false;
            ButtonDebug.IsChecked = false;
        }

        private void OnDebug(object sender, RoutedEventArgs e)
        {
            this.currentView = ViewType.Debug;
            UpdateView();
            ButtonBlog.IsChecked = false;
            ButtonSim.IsChecked = false;
            ButtonControl.IsChecked = false;
            this.strips.Focus();
            this.strips.Show();
        }

        bool fullScreen = true;

        protected override void OnPreviewMouseMove(MouseEventArgs e)
        {
            StartDelayedSleep(InteractiveSleepDelay);
            base.OnPreviewMouseMove(e);
        }

        protected override void OnPreviewStylusDown(StylusDownEventArgs e)
        {
            StartDelayedSleep(InteractiveSleepDelay);
            base.OnPreviewStylusDown(e);
        }

        protected override void OnPreviewTouchDown(TouchEventArgs e)
        {
            StartDelayedSleep(InteractiveSleepDelay);
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
            this.ScreenSaver.Start();
            // a black shield cannot animate over the top of webView!
            webView.Visibility = Visibility.Collapsed;
            this.strips.HidePopup();
            this.controller.HidePopup();
            this.sim.Stop();
        }

        private void OnScreenSaverClosed(object sender, EventArgs e)
        {
            StartDelayedSleep(InteractiveSleepDelay);
        }
    }
}
