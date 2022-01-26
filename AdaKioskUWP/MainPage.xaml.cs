using AdaKiosk;
using AdaSimulation;
using System;
using System.Diagnostics;
using Windows.Storage;
using Windows.System;
using Windows.UI.ViewManagement;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace AdaKioskUWP
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        private DelayedActions actions = new DelayedActions();
        private int InteractiveSleepDelay = 600;
        private int InitialSleepDelay = 600;

        enum ViewType
        {
            Story,
            Simulation,
            Control,
            Debug
        }

        private ViewType currentView = ViewType.Story;

        public MainPage()
        {
            UiDispatcher.Initialize(this.Dispatcher);
            this.InitializeComponent();
            this.Loaded += MainPage_Loaded;
            this.ScreenSaver.Closed += OnScreenSaverClosed;
            this.controller.CommandSelected += OnSendCommand;
            this.sim.SimulatingCommand += OnSimulatingCommand;
            this.strips.CommandSelected += OnSendCommand;
        }

        private void MainPage_Loaded(object sender, RoutedEventArgs e)
        {
            // if nothing happens clear the screen in 10 minutes.
            StartDelayedSleep(InitialSleepDelay);
            this.Focus(FocusState.Programmatic);
        }

        private async void OnSendCommand(object sender, string command)
        {
            try
            {
                //if (bus != null) await bus.SendMessage("server", command);
            }
            catch (Exception ex)
            {
                ShowStatus(ex.Message);
            }
        }

        private void OnSimulatingCommand(object sender, SimulatedCommand e)
        {
            OnSendCommand(sender, e.ToString());
        }

        private void ShowStatus(string status)
        {
            UiDispatcher.Instance.RunOnUIThread(() =>
            {
                TextMessage.Text = status;
            });
        }

        private void StartDelayedSleep(int seconds)
        {
            if (this.ScreenSaver.Visibility == Visibility.Visible)
            {
                ScreenSaver.Stop();
                UpdateView();
            }
            actions.StartDelayedAction("Sleep", OnSleep, TimeSpan.FromSeconds(seconds));
        }

        void OnSleep()
        {
            this.strips.HidePopup();
            this.controller.HidePopup();
            ScreenSaver.Start();
            sim.Stop();
        }

        private void OnScreenSaverClosed(object sender, EventArgs e)
        {
            StartDelayedSleep(InteractiveSleepDelay);
        }

        protected override void OnPointerPressed(PointerRoutedEventArgs e)
        {
            StartDelayedSleep(InteractiveSleepDelay);
            base.OnPointerPressed(e);
            this.Focus(FocusState.Programmatic);
        }

        protected override void OnPointerMoved(PointerRoutedEventArgs e)
        {
            StartDelayedSleep(InteractiveSleepDelay);
            base.OnPointerMoved(e);
        }

        bool fullScreen;

        protected override void OnKeyDown(KeyRoutedEventArgs e)
        {
            StartDelayedSleep(InteractiveSleepDelay);
            if (e.Key == VirtualKey.F11)
            {
                if (!fullScreen)
                {
                    fullScreen = ApplicationView.GetForCurrentView().TryEnterFullScreenMode();
                    Debug.WriteLine("full screen " + fullScreen);
                }
                else
                {
                    fullScreen = false;
                    ApplicationView.GetForCurrentView().ExitFullScreenMode();
                }
                e.Handled = true;
            }
            base.OnPreviewKeyDown(e);
        }

        private void OnBlog(object sender, RoutedEventArgs e)
        {
            this.currentView = ViewType.Story;
            UpdateView();
            ButtonSim.IsChecked = false;
            ButtonControl.IsChecked = false;
            ButtonDebug.IsChecked = false;
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

        private void OnDebug(object sender, RoutedEventArgs e)
        {
            this.currentView = ViewType.Debug;
            UpdateView();
            ButtonBlog.IsChecked = false;
            ButtonSim.IsChecked = false;
            ButtonControl.IsChecked = false;
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
                    this.webView.Visibility = Visibility.Visible;
                    sim.Stop();
                    this.strips.HidePopup();
                    this.controller.HidePopup();
                    break;
                case ViewType.Simulation:
                    this.sim.Visibility = Visibility.Visible;
                    this.sim.Focus(FocusState.Programmatic);
                    this.strips.HidePopup();
                    this.controller.HidePopup();
                    break;
                case ViewType.Control:
                    this.controller.Visibility = Visibility.Visible; ;
                    sim.Stop();
                    this.strips.HidePopup();
                    break;
                case ViewType.Debug:
                    this.strips.Visibility = Visibility.Visible;
                    this.controller.HidePopup();
                    sim.Stop();
                    this.strips.Focus(FocusState.Programmatic);
                    break;
                default:
                    break;
            }
        }

        private void OnSave(KeyboardAccelerator sender, KeyboardAcceleratorInvokedEventArgs args)
        {

        }
    }
}
