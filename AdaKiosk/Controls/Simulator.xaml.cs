// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using AdaKiosk;
using AdaSimulation;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Media.Effects;
using System.Windows.Shapes;

namespace AdaKiosk.Controls
{
    public class SimulatedCommand
    {
    }

    public class ZoneColorEvent : SimulatedCommand
    {
        public int Zone;
        public Color Color;
        public override string ToString()
        {
            return $"/zone/{this.Zone}/{this.Color.R},{this.Color.G},{this.Color.B}";
        }
    }

    public class StripColorEvent : SimulatedCommand
    {
        public string Target;
        public int Strip;
        public Color Color;
        public override string ToString()
        {
            return $"/strip/ada{this.Target}/{this.Strip}/{this.Color.R},{this.Color.G},{this.Color.B}";
        }
    }

    /// <summary>
    /// Interaction logic for Simulator.xaml
    /// </summary>
    public partial class Simulator : UserControl
    {
        List<RaspberryPi> pis = new List<RaspberryPi>();
        ColumnMapping mapping;
        DelayedActions actions = new DelayedActions();
        const double MaxDelay = 5.0;
        List<Path> centerCore = new List<Path>();
        bool stopped = true;
        public event EventHandler<SimulatedCommand> SimulatingCommand;
        ServerConfig config;
        Random random;


        public Simulator()
        {
            UiDispatcher.Initialize();
            InitializeComponent();
            MainGrid.SizeChanged += MainGrid_SizeChanged;
            random = new Random(Environment.TickCount);
            this.Loaded += OnLoaded;
            CenterLeds();
            CreateCenterCore();
            this.IsVisibleChanged += Simulator_IsVisibleChanged;
        }

        private void Simulator_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (this.Visibility != Visibility.Visible)
            {
                Stop();
            }
        }

        public void Start()
        {
            stopped = false;
            CosmosLabel.Foreground = Brushes.Green;
            OnNextEmition();
        }

        public void Stop()
        {
            nextIndex = 0;
            stopped = true;
            CosmosLabel.Foreground = Brushes.Gray;
        }

        internal void HandleMessage(Message message)
        {
            if (message.User == "server")
            {
                // fun times, now to simulate what the server is doing...
                if (!string.IsNullOrEmpty(message.Text))
                {
                    // great these are the simplest to simualte...
                    string[] parts = message.Text.Split('/', StringSplitOptions.RemoveEmptyEntries);
                    if (parts.Length > 1)
                    {
                        switch (parts[0])
                        {
                            case "emotion":
                                SimulateEmotion(parts[1]);
                                break;
                            default:
                                break;
                        }
                    }
                }
                else if (message.Commands != null)
                {
                    foreach (var cmd in message.Commands)
                    {
                        SimulateCommand(cmd);
                    }
                }
            }
        }

        public void SimulateCommand(Command cmd)
        {
            if (cmd.command == "CrossFade" || cmd.command == "Gradient")
            {
                if (cmd.Colors != null)
                {
                    foreach (var pi in pis)
                    {
                        if ("ada" + pi.Name == cmd.Target || string.IsNullOrEmpty(cmd.Target))
                        {
                            pi.AnimateGradient(cmd.Colors, cmd.Seconds);
                        }
                    }
                }
            }
            else if (cmd.command == "MovingGradient")
            {
                // we don't do the moving part yet... although it could be done by animating the GradientStop Offsets.
                foreach (var pi in pis)
                {
                    if ("ada" + pi.Name == cmd.Target || string.IsNullOrEmpty(cmd.Target))
                    {
                        pi.AnimateGradient(cmd.Colors, cmd.Seconds);
                    }
                }
            }
            else if (cmd.command == "ColumnFade")
            {
                if (cmd.Columns != null)
                {
                    foreach (var c in cmd.Columns)
                    {
                        foreach (var pi in pis)
                        {
                            if ("ada" + pi.Name == cmd.Target || string.IsNullOrEmpty(cmd.Target))
                            {
                                pi.AnimateStrip(c.Index, c.Color, cmd.Seconds);
                            }
                        }
                    }
                }
            }
            else if (cmd.command == "Rainbow")
            {
                Color[] colors = GetRainbow();
                // we don't do the moving part yet... although it could be done by animating the GradientStop Offsets.
                foreach (var pi in pis)
                {
                    if ("ada" + pi.Name == cmd.Target || string.IsNullOrEmpty(cmd.Target))
                    {
                        pi.AnimateGradient(colors, cmd.Seconds);
                    }
                }
            }
            else if (cmd.command == "NeuralDrop")
            {
                Color[] colors = GetNeuralDropColors();
                // we don't do the moving part yet... although it could be done by animating the GradientStop Offsets.
                foreach (var pi in pis)
                {
                    if ("ada" + pi.Name == cmd.Target || string.IsNullOrEmpty(cmd.Target))
                    {
                        pi.AnimateGradient(colors, cmd.Seconds);
                    }
                }
            }
        }

        Random rand = new Random(Environment.TickCount);

        private Color[] GetNeuralDropColors()
        {
            Color base_color = Color.FromRgb(0x1b, 0x23, 0x4b);
            List<Color> bubble_colors = new List<Color>();
            const int bubble_size = 16;
            float start = 0;
            float step = (float)(Math.PI / 15);
            for (int i = 0; i < bubble_size; i++)
            {
                int temperature = (int)(128 * Math.Pow(Math.Sin(start), (float)2.0) + 8);
                byte nc = (byte)(temperature);
                bubble_colors.Add(Color.FromRgb(nc, 0, nc));
                start += step;
            }

            // put it in some random location
            int index = rand.Next(20, 70);
            List<Color> colors = new List<Color>();
            // simulate 100 pixels.
            for (int i = 0; i < 100; i++)
            {
                int j = i - index;
                if (j < 0 || j >= bubble_size)
                {
                    colors.Add(base_color);
                }
                else
                {
                    colors.Add(bubble_colors[j]);
                }
            }
            return colors.ToArray();
        }

        private Color[] GetRainbow()
        {
            const int count = 17;
            Color[] colors = new Color[count];
            Color[] cycle = new Color[] { Colors.Red, Colors.Blue, Colors.Green };
            int j = 0;
            for (int i = 0; i < count; i++)
            {
                colors[i] = cycle[j++];
                if (j == cycle.Length) j = 0;
            }
            return colors;
        }

        private void SimulateEmotion(string name)
        {
            Color c = this.config.EmotionColors.GetColor(name);
            SimulateColor(c);
        }

        private void SimulateColor(Color c)
        {
            foreach (var pi in pis)
            {
                pi.AnimateColor(c);
            }
        }

        void CenterLeds()
        {
            Rect bounds = new Rect();
            bool first = true;
            foreach (Path item in AdaCanvas.Children)
            {
                PathGeometry g = item.Data as PathGeometry;
                if (first)
                {
                    bounds = g.Bounds;
                    first = false;
                }
                else
                {
                    bounds = Rect.Union(bounds, g.Bounds);
                }
            }

            // now subtract the top left.
            Vector offset = new Vector(-bounds.Left, -bounds.Top);
            AdaCanvas.Width = bounds.Width;
            AdaCanvas.Height = bounds.Height;

            foreach (Path item in AdaCanvas.Children)
            {
                PathGeometry g = item.Data as PathGeometry;
                PathFigure f = g.Figures[0];
                f.StartPoint = f.StartPoint + offset;
                foreach (var s in f.Segments)
                {
                    LineSegment line = s as LineSegment;
                    if (line != null)
                    {
                        line.Point = line.Point + offset;
                    }
                    else
                    {
                        PolyLineSegment poly = s as PolyLineSegment;
                        if (poly != null)
                        {
                            for (int i = 0; i < poly.Points.Count; i++)
                            {
                                Point pt = poly.Points[i];
                                poly.Points[i] = pt + offset;
                            }
                        }
                        else
                        {
                            //???
                        }
                    }
                }
            }
            AdaCanvas.InvalidateArrange();
        }

        int nextIndex = 0;
        private void AnimateNextStrip()
        {
            Path p = AdaCanvas.Children[nextIndex++] as Path;
            if (nextIndex >= AdaCanvas.Children.Count)
            {
                nextIndex = 0;
            }
            SelectPath(p);
        }

        protected override void OnPreviewKeyDown(KeyEventArgs e)
        {
            if (e.Key == Key.F5)
            {
                Refresh();
            }
            else if (e.Key == Key.F8)
            {
                editing_zone_map = true;
                Watermark.Text = "Editing";
            }
            else if (e.Key == Key.F3)
            {
                if (editing_zone_map)
                {
                    ClearKitchenZones();
                }
            }
            else if (e.Key == Key.OemPlus || e.Key == Key.Add)
            {
                AnimateNextStrip();
            }
            base.OnPreviewKeyDown(e);
        }

        void SaveMaps()
        {
            foreach (var pi in pis)
            {
                pi.Map.Save(pi.Map.FileName);
            }
        }

        void Refresh()
        {
            var brush = new SolidColorBrush(Color.FromRgb(30, 30, 30));
            foreach (Path path in AdaCanvas.Children)
            {
                path.Stroke = brush;
            }
        }

        void ClearKitchenZones()
        {
            if (currentKitchen != null)
            {
                int zone = GetKitchenZone(currentKitchen);
                foreach (var pi in pis)
                {
                    pi.Map.RemoveZone(zone);
                }
            }
        }

        internal void Save()
        {
            if (editing_zone_map)
            {
                this.SaveMaps();
                editing_zone_map = false;
                Watermark.Text = "Simulation";
            }
        }

        private void OnLoaded(object sender, RoutedEventArgs e)
        {
            string path = System.IO.Path.GetDirectoryName(this.GetType().Assembly.Location);
            this.config = ServerConfig.Load(System.IO.Path.Combine(path, "config.json"));

            while (path != null)
            {
                var filename = System.IO.Path.Combine(path, "zone_map_1.json");
                if (System.IO.File.Exists(filename))
                {
                    LoadZoneMaps(path);
                    break;
                }
                path = System.IO.Path.GetDirectoryName(path);
            }

            if (pis.Count == 0)
            {
                MessageBox.Show("Can't find the zone_map_* files", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
            else if (mapping == null)
            {
                MessageBox.Show("Can't find the column_map.json", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void CreateCenterCore()
        {
            for (int i = 0; i < 12; i++)
            {
                // these will be fixed up on resize with coordinates that match the screen.
                var p = new Path();
                p.Name = "Core_" + i;
                this.centerCore.Add(p);
                AdaCanvas.Children.Add(p);
            }
        }

        void OnNextEmition()
        {
            if (pis.Count == 0 || stopped)
            {
                return;
            }

            if (this.ActualWidth > 0)
            {
                int zone = random.Next(0, 6);
                var color = GetRandomEmotion();
                SimulatingCommand?.Invoke(this, new ZoneColorEvent() { Zone = zone, Color = color });
                Grid[] zones = new Grid[] { Zone1, Zone2, Zone3, Zone4, Zone5, Zone6 };
                AddAnimation(zones[zone], color);
            }
            actions.StartDelayedAction("animate", OnNextEmition, TimeSpan.FromSeconds(random.NextDouble() * MaxDelay));
        }

        private void LoadZoneMaps(string path)
        {
            for (int i = 1; i <= 3; i++)
            {
                var name = string.Format("pi{0}", i);
                var filename = string.Format("zone_map_{0}.json", i);
                var fullPath = System.IO.Path.Combine(path, filename);
                pis.Add(new RaspberryPi() { Name = name, Map = ZoneMap.Load(fullPath) });
            }

            this.mapping = ColumnMapping.Load();
            if (mapping != null)
            {
                int total = 0;
                foreach (var row in mapping.Columns)
                {
                    total += row.length;
                }
                Debug.WriteLine(string.Format("Found {0} strips and a total of {1} leds", mapping.Columns.Count, total));

                // now match the zone map with our Path definitions in AdaCanvas.
                int index = 0;
                foreach (var c in AdaCanvas.Children)
                {
                    Path child = c as Path;
                    if (child == null)
                    {
                        continue;
                    }
                    if (index >= mapping.Columns.Count)
                    {
                        Debug.WriteLine("Missing column " + index);
                    }
                    else
                    {
                        var map = mapping.Columns[index]; // Assumes column_map.json is sorted by index property.
                        RaspberryPi pi = pis[map.pi - 1];
                        pi.Load(child, map.col);
                        child.ToolTip = string.Format("index {0}, pi {1}, col {2}", map.index, map.pi, map.col);
                    }
                    index++;
                }
            }
        }

        private void MainGrid_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            double width = e.NewSize.Width;
            double height = e.NewSize.Height;
            double margin = 400;
            if (width < 400)
            {
                margin = 0;
            }
            AdaViewBox.Width = width - margin;

            // setup the channel lines.
            double ellipseWidth = Zone1.ActualWidth; // they are all the same
            double ellipseHeight = Zone1.ActualHeight; // they are all the same
            Rect kitchen1Bounds = Zone1.TransformToAncestor(MainGrid).TransformBounds(new Rect(0, 0, ellipseWidth, ellipseHeight));
            Rect kitchen2Bounds = Zone2.TransformToAncestor(MainGrid).TransformBounds(new Rect(0, 0, ellipseWidth, ellipseHeight));
            Rect kitchen3Bounds = Zone3.TransformToAncestor(MainGrid).TransformBounds(new Rect(0, 0, ellipseWidth, ellipseHeight));
            Rect kitchen4Bounds = Zone4.TransformToAncestor(MainGrid).TransformBounds(new Rect(0, 0, ellipseWidth, ellipseHeight));
            Rect kitchen5Bounds = Zone5.TransformToAncestor(MainGrid).TransformBounds(new Rect(0, 0, ellipseWidth, ellipseHeight));
            Rect kitchen6Bounds = Zone6.TransformToAncestor(MainGrid).TransformBounds(new Rect(0, 0, ellipseWidth, ellipseHeight));
            Rect cosmosDBBounds = CosmosDB.TransformToAncestor(MainGrid).TransformBounds(new Rect(0, 0, ellipseWidth, ellipseHeight));

            LeftChannel1.X2 = LeftChannel1.X1 = LeftChannel2.X2 = LeftChannel2.X1 = kitchen1Bounds.Left + ellipseWidth / 2;
            LeftChannel1.Y1 = kitchen5Bounds.Bottom;
            LeftChannel1.Y2 = kitchen3Bounds.Top;

            LeftChannel2.Y1 = kitchen3Bounds.Bottom;
            LeftChannel2.Y2 = kitchen1Bounds.Top;

            RightChannel1.X1 = RightChannel1.X2 = RightChannel2.X1 = RightChannel2.X2 = kitchen2Bounds.Left + ellipseWidth / 2;
            RightChannel1.Y1 = kitchen6Bounds.Bottom;
            RightChannel1.Y2 = kitchen4Bounds.Top;

            RightChannel2.Y1 = kitchen4Bounds.Bottom;
            RightChannel2.Y2 = kitchen2Bounds.Top;

            BottomChannel1.Y1 = BottomChannel1.Y2 = BottomChannel2.Y1 = BottomChannel2.Y2 = height - (ellipseHeight / 2);
            BottomChannel1.X1 = kitchen1Bounds.Right;
            BottomChannel1.X2 = cosmosDBBounds.Left;

            BottomChannel2.X1 = cosmosDBBounds.Right;
            BottomChannel2.X2 = kitchen2Bounds.Left;

            // Add the center core lines
            double xradius = AdaCanvas.Width / 7;
            double yradius = AdaCanvas.Height / 7;
            double cx = AdaCanvas.Width / 2;
            double cy = AdaCanvas.Height / 2;
            int index = 0;
            for (double angle = 0; angle < 360; angle += 30)
            {
                double x = xradius * Math.Cos(angle * Math.PI / 180);
                double y = yradius * Math.Sin(angle * Math.PI / 180);
                Path p = this.centerCore[index++];
                p.Data = new PathGeometry(new PathFigure[]
                {
                    new PathFigure(new Point(cx, cy), new PathSegment[]
                    {
                        new LineSegment(new Point(cx + x, cy + y), true)
                    },
                    false)
                });
            }
        }

        protected override void OnPreviewMouseDown(MouseButtonEventArgs e)
        {
            // hit point in canvas coordinates
            Point hitPoint = e.GetPosition(AdaCanvas);
            Path path = HitLedStrip(hitPoint);
            if (path != null)
            {
                Debug.WriteLine("Selecting path: " + path.Name);
                SelectPath(path);
            }
            base.OnPreviewMouseDown(e);
        }

        Path HitLedStrip(Point hitPoint)
        {
            // Hit test radius in screen distance needs to be increased when zoomed out.
            double radius = 5;

            // Expand the hit test area by creating a geometry centered on the hit test point.
            var expandedHitTestArea = new GeometryHitTestParameters(new EllipseGeometry(hitPoint, radius, radius));

            Path path = null;

            // The following callback is called for every visual intersecting with the test area, in reverse z-index order.
            // If hitLinks is true, all links passing through the test area are considered, and the closest to the hitPoint is returned in visual.
            // The hit test search is stopped as soon as the first non-link graph object is encountered.
            var hitTestResultCallback = new HitTestResultCallback(r =>
            {
                Path v = r.VisualHit as Path;
                if (v != null && path == null)
                {
                    path = v;
                    return HitTestResultBehavior.Stop;
                }
                return HitTestResultBehavior.Continue;
            });

            // start the search
            VisualTreeHelper.HitTest(AdaCanvas, null, hitTestResultCallback, expandedHitTestArea);
            return path;
        }


        Path selected = null;
        bool editing_zone_map = false;

        void SelectPath(Path path)
        {
            if (selected != null)
            {
                selected.Effect = null;
            }
            selected = path;
            selected.Effect = new DropShadowEffect() { BlurRadius = 15, Color = Colors.White, Opacity = 0.8 };
            Debug.WriteLine(selected.ToolTip);

            if (editing_zone_map)
            {
                if (currentKitchen != null)
                {
                    int zone = GetKitchenZone(currentKitchen);
                    foreach (var pi in pis)
                    {
                        if (pi.HasPath(path))
                        {
                            pi.AddToZone(path, zone);
                            path.Stroke = new SolidColorBrush(currentColor);
                        }
                    }
                }
            }
            else
            {
                foreach (var pi in pis)
                {
                    if (pi.HasPath(path))
                    {
                        int column = pi.GetColumn(path);
                        Color color = currentColor;
                        path.Stroke = new SolidColorBrush(color);
                        SimulatingCommand?.Invoke(this, new StripColorEvent() { Target = pi.Name, Strip = column, Color = color });
                    }
                }
            }
        }

        Grid currentKitchen;
        Color currentColor = Colors.Red;

        private void OnKitchenClick(object sender, MouseButtonEventArgs e)
        {
            AddAnimation((Grid)sender, GetRandomEmotion());
        }

        void AddAnimation(Grid zone, Color color)
        {
            currentColor = color;
            currentKitchen = zone;
            AnimationEmotion(currentKitchen, currentColor);
        }

        void AnimationEmotion(Grid kitchen, Color color)
        {
            Rect kitchenBounds = Zone1.TransformToAncestor(MainGrid).TransformBounds(new Rect(0, 0, kitchen.ActualWidth, kitchen.ActualHeight));

            GlowyBall ball = new GlowyBall(color, GetKitchenZone(kitchen));
            LineArt.Children.Add(ball);
            ball.StartAnimation((string)kitchen.Tag);
            ball.Completed += OnBallCompleted;

            foreach (var child in kitchen.Children)
            {
                if (child is Ellipse)
                {
                    Ellipse e = (Ellipse)child;
                    SolidColorBrush animatingBackground = new SolidColorBrush(color);
                    e.Fill = animatingBackground;
                    animatingBackground.BeginAnimation(SolidColorBrush.ColorProperty, new ColorAnimation(color, Colors.Transparent, new Duration(TimeSpan.FromSeconds(1))));
                }
            }
        }

        private void OnBallCompleted(object sender, EventArgs e)
        {
            GlowyBall ball = (GlowyBall)sender;
            LineArt.Children.Remove(ball);
            // todo: lightup this zone.
            foreach (var pi in pis)
            {
                pi.SetZone(ball.Zone, ball.Color);
            }
        }

        int GetKitchenZone(Grid kitchen)
        {
            string zone = kitchen.Name.Substring(4);
            int result = 0;
            if (int.TryParse(zone, out result))
            {
                result--;
            }
            return result;
        }

        Color GetRandomEmotion()
        {
            return Color.FromRgb((byte)random.Next(0, 255), (byte)random.Next(0, 255), (byte)random.Next(0, 255));
        }

        private void OnFaceClick(object sender, MouseButtonEventArgs e)
        {

        }

        private void OnCosmosClick(object sender, MouseButtonEventArgs e)
        {
            // toggle active animations
            if (stopped)
            {
                Start();
            }
            else
            {
                Stop();
            }
        }
    }
}
