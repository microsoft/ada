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
        const int BubbleMoveDelay = 10;

        public event EventHandler Closed;

        public ScreenSaver()
        {
            this.InitializeComponent();
        }

        public void Start()
        {
            this.Bubble.Visibility = Visibility.Collapsed;
            this.Visibility = Visibility.Visible;
            SolidColorBrush brush = new SolidColorBrush() { Color = Colors.Transparent };
            ScreenBackground.Background = brush;
            var animation = new ColorAnimation() { To = Colors.Black, Duration = new Duration(TimeSpan.FromSeconds(5)) };
            Storyboard storyboard = new Storyboard();
            Storyboard.SetTarget(animation, brush);
            Storyboard.SetTargetProperty(animation, new PropertyPath(SolidColorBrush.ColorProperty));
            storyboard.Children.Add(animation);
            storyboard.Begin();
            storyboard.Completed += OnFadeCompleted;
        }

        protected override void OnTouchDown(TouchEventArgs e)
        {
            actions.StartDelayedAction("event", SendClosedEvent, TimeSpan.FromMilliseconds(1));
            base.OnTouchDown(e);
        }

        protected override void OnPreviewDragOver(DragEventArgs e)
        {
            actions.StartDelayedAction("event", SendClosedEvent, TimeSpan.FromMilliseconds(1));
            base.OnPreviewDragOver(e);
        }

        protected override void OnPreviewKeyDown(KeyEventArgs e)
        {
            actions.StartDelayedAction("event", SendClosedEvent, TimeSpan.FromMilliseconds(1));
            base.OnPreviewKeyDown(e);
        }

        void SendClosedEvent()
        {
            if (Closed != null)
            {
                Closed(this, EventArgs.Empty);
            }
        }

        private void OnFadeCompleted(object sender, object e)
        {
            this.RandomBubblePlacement();
            this.Bubble.Visibility = Visibility.Visible;
        }

        private void RandomBubblePlacement()
        {
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
