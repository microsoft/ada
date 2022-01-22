using AdaSimulation;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace AdaKiosk
{
    /// <summary>
    /// Interaction logic for StripControl.xaml
    /// </summary>
    public partial class StripControl : UserControl
    {
        ColumnMap _column;

        public StripControl()
        {
            InitializeComponent();
            this.Loaded += StripControl_Loaded;
        }

        public ColumnMap Column
        {
            get => _column;
            set
            {
                if (_column != null)
                {
                    _column.PropertyChanged -= OnColumnPropertyChanged;
                }
                _column = value;
                if (_column != null)
                {
                    _column.PropertyChanged += OnColumnPropertyChanged;
                }
            }
        }

        private void OnColumnPropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            if (e.PropertyName == "offset" || e.PropertyName == "LedWidth")
            {
                LedPanel.Margin = new Thickness(Column.offset * Column.LedWidth, 0, 0, 0);
            }
        }

        private void StripControl_Loaded(object sender, RoutedEventArgs e)
        {
            if (Column != null)
            {
                this.StripIndex.Text = Column.index.ToString();
                this.PiIndex.Text = Column.pi.ToString();
                this.ColumnIndex.Text = Column.col.ToString();
                foreach (var led in Column.leds)
                {
                    LedPanel.Children.Add(new LedControl() { Model = led });
                }
                LedPanel.Margin = new Thickness(Column.offset * Column.LedWidth, 0, 0, 0);
            }
        }
    }
}
