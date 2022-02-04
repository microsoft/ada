using AdaSimulation;
using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Animation;

// The User Control item template is documented at https://go.microsoft.com/fwlink/?LinkId=234236

namespace AdaKiosk.Controls
{
    public sealed partial class ScreenSaver : UserControl
    {
        DelayedActions actions = new DelayedActions();
        Random rand = new Random(Environment.TickCount);
        const int FadeDelay = 5;
        const int BubbleMoveDelay = 10;
        int startTick = 0;

        public event EventHandler<string> Closed;

        public ScreenSaver()
        {
            this.InitializeComponent();
        }

        public void Start()
        {
            startTick = Environment.TickCount;
            this.Bubble.Visibility = Visibility.Collapsed;
            this.Visibility = Visibility.Visible;
            SolidColorBrush brush = new SolidColorBrush() { Color = Colors.Transparent };
            ScreenBackground.Background = brush;
            var animation = new ColorAnimation() { To = Colors.Black, Duration = new Duration(TimeSpan.FromSeconds(FadeDelay)) };
            brush.BeginAnimation(SolidColorBrush.ColorProperty, animation);
            actions.StartDelayedAction("move", RandomBubblePlacement, TimeSpan.FromSeconds(BubbleMoveDelay));
        }

        protected override void OnTouchDown(TouchEventArgs e)
        {
            e.Handled = true;
            if (Environment.TickCount > startTick + 5000)
            {
                actions.StartDelayedAction("event", () => SendClosedEvent("Touch"), TimeSpan.FromMilliseconds(10));
            }
            base.OnTouchDown(e);
        }

        protected override void OnPreviewMouseMove(MouseEventArgs e)
        {
            e.Handled = true;
            if (Environment.TickCount > startTick + 5000)
            {
                actions.StartDelayedAction("event", () => SendClosedEvent("Mouse"), TimeSpan.FromMilliseconds(10));
            }
            base.OnPreviewMouseMove(e);
        }

        protected override void OnPreviewKeyDown(KeyEventArgs e)
        {
            e.Handled = true;
            if (Environment.TickCount > startTick + 5000)
            {
                actions.StartDelayedAction("event", () => SendClosedEvent("Keyboard"), TimeSpan.FromMilliseconds(10));
            }
            base.OnPreviewKeyDown(e);
        }

        void SendClosedEvent(string name)
        {
            if (Closed != null)
            {
                Closed(this, name);
            }
        }

        private void RandomBubblePlacement()
        {
            this.Bubble.Visibility = Visibility.Visible;
            var xrange = this.ActualWidth - this.Bubble.Width;
            var yrange= this.ActualHeight - this.Bubble.Height;
            var x = rand.Next(0, (int)xrange);
            var y = rand.Next(0, (int)yrange);
            this.Bubble.Margin = new Thickness(x, y, 0, 0);
            actions.StartDelayedAction("move", RandomBubblePlacement, TimeSpan.FromSeconds(BubbleMoveDelay));
        }

        public void Stop()
        {
            this.Visibility = Visibility.Collapsed;
            this.actions.Close();
        }
    }
}
