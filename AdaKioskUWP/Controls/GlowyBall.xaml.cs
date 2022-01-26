﻿using System;
using System.Collections.Generic;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Animation;
using Windows.UI.Xaml.Shapes;
using Windows.UI.Composition;
using Windows.UI.Xaml.Hosting;
using System.Numerics;

// The User Control item template is documented at https://go.microsoft.com/fwlink/?LinkId=234236

namespace AdaKioskUWP.Controls
{
    public sealed partial class GlowyBall : UserControl
    {
        List<Line> path;
        List<double> segments;
        double totalLength;
        Storyboard story;
        const int BallSize = 25;
        const int BlurRadius = 20;
        int zone;
        Color color;
        List<bool> reversed;
        Ellipse ellipse;
        Visual shadow;

        public GlowyBall()
        {
            this.InitializeComponent();
            ellipse = new Ellipse() { Width = BallSize, Height = BallSize };
            this.Content = ellipse;
        }

        public Color Color { get => color; set { this.color = value; ellipse.Fill = new SolidColorBrush(value); } }

        public int Zone { get => zone; set => this.zone = value; }

        public event EventHandler Completed;

        public double PositionOnPath
        {
            get { return (double)GetValue(PositionOnPathProperty); }
            set { SetValue(PositionOnPathProperty, value); }
        }

        // Using a DependencyProperty as the backing store for PositionOnPath.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty PositionOnPathProperty =
            DependencyProperty.Register("PositionOnPath", typeof(double), typeof(GlowyBall), new PropertyMetadata(0.0, new PropertyChangedCallback(OnPositionChanged)));

        private static void OnPositionChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            ((GlowyBall)d).OnPositionChanged();
        }

        private void OnPositionChanged()
        {
            double pos = PositionOnPath;
            double start = 0;
            int index = 0;

            foreach (double s in segments)
            {
                if (pos >= start && pos <= start + s)
                {
                    // found the segment, so interpolate the position on this segment.
                    Line line = path[index];
                    double x = 0;
                    double y = 0;
                    if (reversed[index])
                    {
                        double dx = line.X1 - line.X2;
                        double dy = line.Y2 - line.Y1;
                        double percent = (pos - start) / s;
                        x = line.X2 + (percent * dx) - BallSize / 2;
                        y = line.Y1 + (percent * dy) - BallSize / 2;
                    }
                    else
                    {
                        double dx = line.X2 - line.X1;
                        double dy = line.Y2 - line.Y1;
                        double percent = (pos - start) / s;
                        x = line.X1 + (percent * dx) - BallSize / 2;
                        y = line.Y1 + (percent * dy) - BallSize / 2;
                    }
                    Canvas.SetLeft(this, x);
                    Canvas.SetTop(this, y);
                    return;
                }
                else
                {
                    start += s;
                }
                index++;
            }
        }

        internal void StartAnimation(string tag)
        {
            if (shadow == null)
            {
                shadow = ElementCompositionPreview.GetElementVisual(ellipse);
                Compositor compositor = shadow.Compositor;

                // Create a drop shadow
                var dropShadow = compositor.CreateDropShadow();
                dropShadow.Color = this.color;
                dropShadow.BlurRadius = BlurRadius;
                dropShadow.Offset = new Vector3(0f, 0f, 0.0f);

                var shadowVisual = compositor.CreateSpriteVisual();
                shadowVisual.Shadow = dropShadow;

                // Add the shadow as a child of the host in the visual tree
                ElementCompositionPreview.SetElementChildVisual(ellipse, shadowVisual);

                // Make sure size of shadow host and shadow visual always stay in sync
                var bindSizeAnimation = compositor.CreateExpressionAnimation("hostVisual.Size");
                bindSizeAnimation.SetReferenceParameter("hostVisual", shadow);
                shadowVisual.StartAnimation("Size", bindSizeAnimation);
            }
            LoadPath(tag);

            Canvas parent = this.Parent as Canvas;
            double width = parent.ActualWidth / 2; // want to do the whole thing about 2 seconds
            double seconds = totalLength / width;

            story = new Storyboard();
            Storyboard.SetTarget(story, this);
            Storyboard.SetTargetProperty(story, "PositionOnPath");
            story.Children.Add(new DoubleAnimation() { From = 0, To = totalLength, Duration = new Duration(TimeSpan.FromSeconds(seconds)), EnableDependentAnimation=true } );
            story.Completed += OnStoryCompleted;
            story.Begin();
        }

        private void LoadPath(string tag)
        {
            path = new List<Line>();
            segments = new List<double>();
            reversed = new List<bool>();
            totalLength = 0;

            Canvas parent = this.Parent as Canvas;
            foreach (var id in tag.Split(','))
            {
                bool r = false;
                string name = id.Trim();
                if (name.StartsWith("-"))
                {
                    r = true;
                    name = name.Trim('-');
                }
                Line line = (Line)parent.FindName(name.Trim());
                if (line != null)
                {
                    path.Add(line);
                    reversed.Add(r);
                    double dx = (line.X2 - line.X1);
                    double dy = (line.Y2 - line.Y1);
                    double length = Math.Sqrt((dx * dx) + (dy * dy));
                    segments.Add(length);
                    totalLength += length;
                }
            }
        }
        
        private void OnStoryCompleted(object sender, object e)
        {
            // remove it!
            story.Stop();
            if (Completed != null)
            {
                Completed(this, EventArgs.Empty);
            }
        }
    }
}
