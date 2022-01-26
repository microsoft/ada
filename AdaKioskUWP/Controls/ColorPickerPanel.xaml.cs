using AdaKioskUWP.Utilities;
using AdaSimulation;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading.Tasks;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.System;
using Windows.UI;
using Windows.UI.Popups;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Navigation;

// The User Control item template is documented at https://go.microsoft.com/fwlink/?LinkId=234236

namespace AdaKioskUWP.Controls
{
    public sealed partial class ColorPickerPanel : UserControl
    {
        public event EventHandler ColorChanged;
        public event EventHandler Accept;
        public event EventHandler Cancel;
        public event EventHandler<Exception> Error;
        bool isDown;

        public ColorPickerPanel()
        {
            this.InitializeComponent();
            DataContext = this;
            ColorPanel.PointerPressed += GrayScale_PointerPressed;
            ColorPanel.PointerMoved += GrayScale_PointerMoved;
            ColorPanel.PointerReleased += GrayScale_PointerReleased;
        }

        private void GrayScale_PointerMoved(object sender, PointerRoutedEventArgs e)
        {
            if (isDown)
            {
                SetColorAt(e.GetCurrentPoint(ColorPanel).Position);
            }
        }

        private void GrayScale_PointerPressed(object sender, PointerRoutedEventArgs e)
        {
            isDown = true;
            SetColorAt(e.GetCurrentPoint(ColorPanel).Position);
            this.Focus(FocusState.Programmatic);
        }

        private void GrayScale_PointerReleased(object sender, PointerRoutedEventArgs e)
        {
            isDown = false;
        }

        public Color Color
        {
            get { return (Color)GetValue(ColorProperty); }
            set { SetValue(ColorProperty, value); }
        }

        // Using a DependencyProperty as the backing store for Color.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty ColorProperty =
            DependencyProperty.Register("Color", typeof(Color), typeof(ColorPickerPanel), new PropertyMetadata(Colors.Transparent, OnColorChanged));

        private static void OnColorChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            Color c = (Color)e.NewValue;
            ColorPickerPanel panel = (ColorPickerPanel)d;
            panel.OnColorChanged(c);
        }

        void OnColorChanged(Color c)
        { 
            HlsColor hls = new HlsColor(c);
            if (!luminanceUpdate)
            {
                this.LuminanceSlider.Value = hls.Luminance * this.LuminanceSlider.Maximum;
            }
            if (!transparencyUpdate)
            {
                this.TransparencySlider.Value = ((double)c.A / 255.0) * this.TransparencySlider.Maximum;
            }
            this.ColorName.Text = c.ToString();

            if (ColorChanged != null)
            {
                ColorChanged(this, EventArgs.Empty);
            }
        }

        protected override Size ArrangeOverride(Size finalSize)
        {
            double width = finalSize.Width;
            double patchSize = width / 4;
            if (patchSize < 50) patchSize = 50;
            Swatch.Width = Swatch.Height = TransparencyPatch.Width = TransparencyPatch.Height = patchSize;
            return base.ArrangeOverride(finalSize);
        }

        private void OnOkButtonClick(object sender, RoutedEventArgs e)
        {
            Accept?.Invoke(this, new EventArgs());
        }

        private void OnCancelButtonClick(object sender, RoutedEventArgs e)
        {
            Cancel?.Invoke(this, new EventArgs());
        }

        protected override void OnPreviewKeyDown(KeyRoutedEventArgs e)
        {
            if (e.Key == VirtualKey.Escape)
            {
                Cancel?.Invoke(this, new EventArgs());
                e.Handled = true;
            }
            else if (e.Key == VirtualKey.Enter)
            {
                Accept?.Invoke(this, new EventArgs());
                e.Handled = true;
            }
            base.OnPreviewKeyDown(e);
        }

        bool luminanceUpdate;

        private void LuminanceSlider_ValueChanged(object sender, RangeBaseValueChangedEventArgs e)
        {
            Color c = this.Color;
            HlsColor hls = new HlsColor(c);
            hls.Luminance = (float)LuminanceSlider.Value / LuminanceSlider.Maximum;
            Color nc = hls.RgbColor;
            luminanceUpdate = true;
            this.Color = Color.FromArgb(c.A, nc.R, nc.G, nc.B);
            luminanceUpdate = false;
        }

        bool transparencyUpdate;

        private void TransparencySlider_ValueChanged(object sender, RangeBaseValueChangedEventArgs e)
        {
            Color c = this.Color;
            transparencyUpdate = true;
            this.Color = Color.FromArgb((byte)((e.NewValue * 255) / TransparencySlider.Maximum), c.R, c.G, c.B);
            transparencyUpdate = false;
        }        

        RenderTargetBitmap bitmap; // cache
        async Task<RenderTargetBitmap> GetOrCreateBitmap()
        {
            if (ColorPanel.ActualWidth > 0 && ColorPanel.ActualHeight > 0 && bitmap == null)
            {
                bitmap = new RenderTargetBitmap();
                await bitmap.RenderAsync(this.ColorPanel);
            }
            return bitmap;
        }

        private async void SetColorAt(Point pos)
        {
            Exception error = null;
            try
            {
                var bitmap = await GetOrCreateBitmap();
                if (bitmap == null)
                {
                    return;
                }
                byte[] pixel = new byte[4];
                double xscaled = (pos.X * bitmap.PixelWidth) / ColorPanel.ActualWidth;
                double yscaled = (pos.Y * bitmap.PixelHeight) / ColorPanel.ActualHeight;
                xscaled = Math.Max(0, Math.Min(xscaled, bitmap.PixelWidth));
                yscaled = Math.Max(0, Math.Min(yscaled, bitmap.PixelHeight));
                var buffer = await bitmap.GetPixelsAsync(); // BGRA8 format
                uint bytesPerPixel = 4;
                uint stride = bytesPerPixel * (uint)bitmap.PixelWidth;
                uint offset = (uint)xscaled * bytesPerPixel;
                if (yscaled > 0) offset += stride * ((uint)yscaled - 1);
                buffer.CopyTo((uint)offset, pixel, 0, 4);

                // extract BGRA8 components.
                double b = (double)pixel[0];
                double r = (double)pixel[2];
                double g = (double)pixel[1];
                double alpha = (double)pixel[3];
                Color color = Color.FromArgb((byte)alpha, (byte)r, (byte)g, (byte)b);
                this.Color = color;
            }
            catch (Exception ex)
            {
                error = ex;
            }
            if (error != null && Error != null) {
                Error(this, error);            
            }
        }


        private void OnColorNameChanged(object sender, TextChangedEventArgs e)
        {
            Color? c = ColorParser.TryParseColor(this.ColorName.Text);
            if (c.HasValue && this.Color != c.Value)
            {
                this.Color = c.Value;
            }
        }

        private void OnColorPaneSizeChanged(object sender, SizeChangedEventArgs e)
        {
            bitmap = null; // cache is invalid.
        }
    }
}
