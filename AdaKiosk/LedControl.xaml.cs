using AdaSimulation;
using System;
using System.Windows.Controls;
using System.Windows.Media;

namespace AdaKiosk
{
    /// <summary>
    /// Interaction logic for LedControl.xaml
    /// </summary>
    public partial class LedControl : UserControl
    {
        Led model;

        public LedControl()
        {
            InitializeComponent();
        }

        public Led Model
        {
            get
            {
                return model;
            }

            set
            {
                if (model != null)
                {
                    model.PropertyChanged -= OnLedPropertyChanged;
                }
                model = value;

                if (model != null)
                {
                    model.PropertyChanged += OnLedPropertyChanged;
                    UpdateWidth();
                    UpdateColor();
                }
            }
        }

        SolidColorBrush brush;

        private void OnLedPropertyChanged(object sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            if (e.PropertyName == "Color")
            {
                UpdateColor();
            }
            else if (e.PropertyName == "Width")
            {
                UpdateWidth();
            }
        }

        private void UpdateColor()
        {
            if (this.model != null)
            {
                if (brush == null)
                {
                    brush = new SolidColorBrush();
                    Border.Background = brush;
                }
                brush.Color = model.Color;
            }
        }

        private void UpdateWidth()
        {
            if (this.model != null && this.model.Width > 0)
            {
                this.Width = this.model.Width;
            }
        }
    }
}
