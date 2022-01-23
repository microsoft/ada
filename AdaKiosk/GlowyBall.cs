// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Media.Effects;
using System.Windows.Shapes;

namespace AdaSimulation
{
    class GlowyBall : UserControl
    {
        List<Line> path;
        List<double> segments;
        double totalLength;
        Storyboard story;
        const int BallSize = 25;
        int zone;
        Color color;
        List<bool> reversed;

        public GlowyBall(Color color, int zone)
        {
            this.zone = zone;
            this.color = color;
            Ellipse e = new Ellipse() { Width = BallSize, Height = BallSize, Fill = new SolidColorBrush(color) };
            e.Effect = new BlurEffect() { Radius = BallSize };
            this.Content = e;
        }

        public Color Color => color;

        public int Zone => zone;

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
            LoadPath(tag);

            Canvas parent = this.Parent as Canvas;
            double width = parent.ActualWidth / 2; // want to do the whole thing about 2 seconds
            double seconds = totalLength / width;

            story = new Storyboard();
            Storyboard.SetTarget(story, this);
            Storyboard.SetTargetProperty(story, new PropertyPath(PositionOnPathProperty));
            story.Children.Add(new DoubleAnimation(totalLength, new Duration(TimeSpan.FromSeconds(seconds))));
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

        private void OnStoryCompleted(object sender, EventArgs e)
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
