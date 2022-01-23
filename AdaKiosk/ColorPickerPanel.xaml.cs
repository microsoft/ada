﻿// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace AdaSimulation
{
    /// <summary>
    /// Interaction logic for ColorPickerPanel.xaml
    /// </summary>
    public partial class ColorPickerPanel : UserControl
    {
        public ColorPickerPanel()
        {
            InitializeComponent();
            DataContext = this;

            GrayScale.MouseDown += new MouseButtonEventHandler(GrayScale_MouseDown);
            GrayScale.MouseMove += new MouseEventHandler(GrayScale_MouseMove);
            GrayScale.MouseUp += new MouseButtonEventHandler(GrayScale_MouseUp);
        }

        public event EventHandler ColorChanged;
        public event EventHandler Accept;
        public event EventHandler Cancel;

        bool isDown;

        void GrayScale_MouseUp(object sender, MouseButtonEventArgs e)
        {
            isDown = false;
        }

        void GrayScale_MouseMove(object sender, MouseEventArgs e)
        {
            if (isDown)
            {
                SetColorAt(e.GetPosition(GrayScale));
            }
        }

        void GrayScale_MouseDown(object sender, MouseButtonEventArgs e)
        {
            isDown = true;
            SetColorAt(e.GetPosition(GrayScale));
            Focus();
        }

        public Color Color
        {
            get { return (Color)GetValue(ColorProperty); }
            set { SetValue(ColorProperty, value); }
        }

        // Using a DependencyProperty as the backing store for Color.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty ColorProperty =
            DependencyProperty.Register("Color", typeof(Color), typeof(ColorPickerPanel), new UIPropertyMetadata(Colors.Transparent, OnColorChanged, OnCoerceColorValue));

        private static void OnColorChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            Color c = (Color)e.NewValue;
            HlsColor hls = new HlsColor(c);
            ColorPickerPanel panel = (ColorPickerPanel)d;
            panel.LuminanceSlider.Value = hls.Luminance;
            panel.TransparencySlider.Value = (double)c.A / 255.0;
            panel.OnColorChanged();
        }

        void OnColorChanged()
        {
            if (ColorChanged != null)
            {
                ColorChanged(this, EventArgs.Empty);
            }
        }

        protected override Size ArrangeOverride(Size arrangeBounds)
        {
            double width = arrangeBounds.Width;
            double patchSize = width / 4;
            if (patchSize < 50) patchSize = 50;
            Swatch.Width = Swatch.Height = TransparencyPatch.Width = TransparencyPatch.Height = patchSize;
            return base.ArrangeOverride(arrangeBounds);
        }

        private static object OnCoerceColorValue(DependencyObject d, object baseValue)
        {
            Color baseColor = (Color)baseValue;
            return baseValue;
        }

        private void LuminanceSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            Color c = this.Color;
            HlsColor hls = new HlsColor(c);
            hls.Luminance = (float)LuminanceSlider.Value;
            Color nc = hls.RgbColor;
            this.Color = Color.FromArgb(c.A, nc.R, nc.G, nc.B);
        }

        private void TransparencySlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            Color c = this.Color;
            this.Color = Color.FromArgb((byte)(e.NewValue * 255), c.R, c.G, c.B);
        }

        RenderTargetBitmap bitmap; // cache
        RenderTargetBitmap GetOrCreateBitmap()
        {
            if (Rainbow.ActualWidth > 0 && Rainbow.ActualHeight > 0)
            {
                var width = (int)Rainbow.ActualWidth;
                var height = (int)Rainbow.ActualHeight;
                if (bitmap == null || bitmap.PixelHeight != height || bitmap.PixelWidth != width)
                {
                    bitmap = new RenderTargetBitmap(width, height, 96, 96, PixelFormats.Pbgra32);
                    bitmap.Render(this.Rainbow);
                    bitmap.Render(this.GrayScale);
                }
            }
            return bitmap;
        }

        private void SetColorAt(Point pos)
        {
            var bitmap = GetOrCreateBitmap();
            if (bitmap == null)
            {
                return;
            }
            byte[] pixels = new byte[4];
            bitmap.CopyPixels(new Int32Rect((int)Math.Max(0, Math.Min(bitmap.Width - 1, pos.X)), (int)Math.Max(0, Math.Min(bitmap.Height - 1, pos.Y)), 1, 1), pixels, 4, 0);

            // there is a premultiply on the alpha value that we have to reverse.
            double alpha = (double)pixels[3];
            double r = (double)pixels[2];
            double g = (double)pixels[1];
            double b = (double)pixels[0];
            if (alpha != 0)
            {
                r = (r * 255) / alpha;
                g = (g * 255) / alpha;
                b = (b * 255) / alpha;
            }
            Color color = Color.FromArgb((byte)alpha, (byte)r, (byte)g, (byte)b);
            this.Color = color;
        }

        private void OnOkButtonClick(object sender, RoutedEventArgs e)
        {
            Accept?.Invoke(this, new EventArgs());
        }

        protected override void OnPreviewKeyDown(KeyEventArgs e)
        {
            if (e.Key == Key.Escape)
            {
                Cancel?.Invoke(this, new EventArgs());
                e.Handled = true;
            }
            else if (e.Key == Key.Enter)
            {
                Accept?.Invoke(this, new EventArgs());
                e.Handled = true;
            }
            base.OnPreviewKeyDown(e);
        }

        private void OnCancelButtonClick(object sender, RoutedEventArgs e)
        {
            Cancel?.Invoke(this, new EventArgs());
            e.Handled = true;
        }
    }
}
