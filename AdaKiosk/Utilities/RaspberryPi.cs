// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Shapes;

namespace AdaSimulation
{
    // Models one of the Raspberry pi machines, with up to 16 LED strips.
    class RaspberryPi
    {
        public string Name;
        public ZoneMap Map;

        public Dictionary<int, Path> strips = new Dictionary<int, Path>();
        public Dictionary<Path, int> columnMap = new Dictionary<Path, int>();

        public void Load(Path leds, int col)
        {
            strips[col] = leds;
            columnMap[leds] = col;
        }

        internal void SetZone(int zone, Color color)
        {
            int count = 0;
            var brush = new SolidColorBrush(color);
            foreach (var map in Map.ZoneLeds)
            {
                if (map.zone == zone)
                {
                    // light this whole strip for now.
                    count++;
                    int strip = map.col;
                    Path path;
                    if (strips.TryGetValue(strip, out path))
                    {
                        path.Stroke = brush;
                    }
                    else
                    {
                        Debug.WriteLine("Bug in map, strip {0} is missing", strip);
                    }
                }
            }
            Debug.WriteLine("{2}: Found {0} strips in zone {1}", count, zone, Name);
        }

        public bool HasPath(Path path)
        {
            return this.columnMap.ContainsKey(path);
        }

        public void AddToZone(Path path, int zone)
        {
            if (this.columnMap.TryGetValue(path, out int column))
            {
                Map.ZoneLeds.Add(new StripMap() { zone = zone, col = column });
            }
        }

        internal int GetColumn(Path path)
        {
            if (columnMap.TryGetValue(path, out int column))
            {
                return column;
            }
            return 0;
        }

        Color ClampMinimumColor(Color c)
        {
            if (c.R + c.G + c.B < 30)
            {
                c.R += 10;
                c.G += 10;
                c.B += 10;
            }
            return c;
        }

        internal void AnimateColor(Color c, int seconds = 0)
        {
            c = ClampMinimumColor(c);
            foreach (var path in strips.Values)
            {
                if (seconds == 0)
                {
                    path.Stroke = new SolidColorBrush(c);
                }
                else
                {
                    SolidColorBrush brush = path.Stroke as SolidColorBrush;
                    if (brush == null || brush.IsFrozen)
                    {
                        brush = new SolidColorBrush(Colors.Black);
                        path.Stroke = brush;
                    }
                    var animation = new ColorAnimation(c, new System.Windows.Duration(TimeSpan.FromSeconds(seconds)));
                    brush.BeginAnimation(SolidColorBrush.ColorProperty, animation);
                }
            }
        }

        HashSet<string> registeredNames = new HashSet<string>();

        /// <summary>
        /// Animate whole thing with the given uniform gradient.
        /// </summary>
        internal void AnimateGradient(Color[] colors, int seconds)
        {
            if (colors.Length > 1)
            {
                Storyboard story = new Storyboard();
                FrameworkElement parent = null;
                int pathIndex = 0;
                foreach (var path in strips.Values)
                {
                    if (parent == null)
                    {
                        parent = path.Parent as FrameworkElement;
                    }

                    LinearGradientBrush brush = path.Stroke as LinearGradientBrush;
                    if (brush == null || brush.IsFrozen || brush.GradientStops.Count != colors.Length)
                    {
                        brush = new LinearGradientBrush();
                        bool reverse = path.Tag as string == "reverse";
                        if (reverse)
                        {
                            brush.StartPoint = new Point(1, 1);
                            brush.EndPoint = new Point(0, 0);
                        }
                        int i = 0;
                        foreach (Color c in colors)
                        {
                            Color mc = ClampMinimumColor(c);
                            double offset = (double)i / (double)colors.Length;
                            var stop = new GradientStop() { Color = mc, Offset = offset };
                            brush.GradientStops.Add(stop);
                            var name = "GradientStop" + this.Name + "_" + pathIndex + "_" + i;
                            if (registeredNames.Contains(name))
                            {
                                parent.UnregisterName(name);
                            }
                            parent.RegisterName(name, stop);
                            registeredNames.Add(name);
                            i++;
                        }
                        path.Stroke = brush;
                    }

                    int index = 0;
                    foreach (var c in colors)
                    {
                        var animation = new ColorAnimation(c, new System.Windows.Duration(TimeSpan.FromSeconds(seconds)));
                        var name = "GradientStop" + this.Name + "_" + pathIndex + "_" + index;
                        Storyboard.SetTargetName(animation, name);
                        Storyboard.SetTargetProperty(animation, new PropertyPath(GradientStop.ColorProperty));
                        story.Children.Add(animation);
                        index++;
                    }
                    pathIndex++;
                }

                story.Begin(parent);
            }
            else if (colors.Length == 1)
            {
                AnimateColor(colors[0], seconds);
            }
        }

        internal void AnimateStrip(int index, Color color, int seconds)
        {
            color = ClampMinimumColor(color);
            if (this.strips.TryGetValue(index, out Path path))
            {
                if (seconds == 0)
                {
                    path.Stroke = new SolidColorBrush(color);
                }
                else
                {
                    SolidColorBrush brush = path.Stroke as SolidColorBrush;
                    if (brush == null || brush.IsFrozen)
                    {
                        brush = new SolidColorBrush(Colors.Black);
                        path.Stroke = brush;
                    }
                    var animation = new ColorAnimation(color, new System.Windows.Duration(TimeSpan.FromSeconds(seconds)));
                    brush.BeginAnimation(SolidColorBrush.ColorProperty, animation);
                }
            }
        }

        /// <summary>
        /// Animate a specific gradient on a specific strip.
        /// </summary>
        internal void AnimateStripGradient(int index, Color[] colors, int seconds)
        {
            if (this.strips.TryGetValue(index, out Path path))
            {
                if (colors.Length == 1)
                {
                    AnimateStrip(index, colors[0], seconds);
                    return;
                }

                LinearGradientBrush brush = new LinearGradientBrush();
                int pos = 0;
                double max = (double)(colors.Length - 1);
                foreach (var c in colors)
                {
                    double offset = (double)pos / max;
                    var color = ClampMinimumColor(c);
                    brush.GradientStops.Add(new GradientStop(color, offset));
                    pos++;
                }

                if (seconds == 0)
                {
                    path.Stroke = brush;
                }
                else
                {
                    LinearGradientBrush existing = path.Stroke as LinearGradientBrush;
                    if (existing == null || existing.IsFrozen || existing.GradientStops.Count != colors.Length)
                    {
                        path.Stroke = brush;
                        existing = brush;
                    }

                    Storyboard sb = new Storyboard();
                    foreach (var g in brush.GradientStops)
                    {
                        var animation = new ColorAnimation(g.Color, new System.Windows.Duration(TimeSpan.FromSeconds(seconds)));
                        Storyboard.SetTargetProperty(animation, new PropertyPath(GradientStop.ColorProperty));
                        Storyboard.SetTarget(existing, animation);
                        sb.Children.Add(animation);
                    }

                    path.BeginStoryboard(sb);
                }
            }
        }
    }
}
