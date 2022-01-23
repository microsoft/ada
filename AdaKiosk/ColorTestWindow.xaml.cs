// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace AdaSimulation
{
    /// <summary>
    /// Interaction logic for ColorTestWindow.xaml
    /// </summary>
    public partial class ColorTestWindow : Window
    {
        public ColorTestWindow()
        {
            InitializeComponent();
            this.SizeChanged += ColorTestWindow_SizeChanged;
        }

        private void ColorTestWindow_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            var width = (int)e.NewSize.Width; // for example
            var height = (int)e.NewSize.Height; // for example
            var dpiX = 96d;
            var dpiY = 96d;
            var pixelFormat = PixelFormats.Rgb24;
            var bytesPerPixel = (pixelFormat.BitsPerPixel + 7) / 8; // == 1 in this example
            var stride = bytesPerPixel * width; // == width in this example

            byte[] buffer = new byte[width * height * 3];
            int pos = 0;
            double halfHeight = height / 2;
            for (int j = 0; j < height; j++)
            {
                for (int i = 0; i < width; i++)
                {
                    double hue = (double)i * 360.0 / (double)width;
                    double saturation = 0;
                    double luminance = 0;
                    if (j > halfHeight)
                    {
                        // top half is fully bright, ramp saturation
                        luminance = 0.5;
                        saturation = (double)(halfHeight - (j - halfHeight)) / (double)halfHeight;
                    }
                    else
                    {
                        // bottom half is fully saturated, ramp luminance
                        luminance = (double)(halfHeight - j) / (double)halfHeight;
                        saturation = 1;
                    }
                    Color rgb = new HlsColor(hue, luminance, saturation).RgbColor;
                    buffer[pos++] = rgb.R;
                    buffer[pos++] = rgb.G;
                    buffer[pos++] = rgb.B;
                }
            }

            ColorWheel.Source = BitmapSource.Create(width, height, dpiX, dpiY, pixelFormat, null, buffer, stride);
        }
    }
}
