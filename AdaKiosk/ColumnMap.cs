using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Documents;
using System.Windows.Media;

namespace AdaSimulation
{
    [DataContract]
    public class ColumnMapping
    {
        [DataMember(Name="columns")]
        public List<ColumnMap> Columns;

        internal string FileName;

        public static ColumnMapping Instance;

        public ColumnMapping()
        {
            Instance = this;
        }

        public static ColumnMapping Load()
        {
            if (Instance != null)
            {
                return Instance;
            }

            string path = System.IO.Path.GetDirectoryName(typeof(ColumnMapping).Assembly.Location);
            var mapFile = System.IO.Path.Combine(path, "column_map.json");
            if (System.IO.File.Exists(mapFile))
            {
                using (var fs = new FileStream(mapFile, FileMode.Open, FileAccess.Read))
                {
                    DataContractJsonSerializer s = new DataContractJsonSerializer(typeof(ColumnMapping));
                    var result = s.ReadObject(fs) as ColumnMapping;
                    result.FileName = mapFile;

                    int total = 0;
                    foreach (var row in result.Columns)
                    {
                        total += row.length;
                        row.leds = new List<Led>();
                        for (int i = 0; i < row.length; i++)
                        {
                            row.leds.Add(new Led() { index = i, parent = row });
                        }
                    }
                    Debug.WriteLine(string.Format("Found {0} strips and a total of {1} leds", result.Columns.Count, total));
                    Instance = result;
                    return result;
                }
            }
            return Instance;
        }

        public void Save()
        {
            using (var fs = new FileStream(this.FileName, FileMode.Create, FileAccess.Write))
            {
                DataContractJsonSerializer s = new DataContractJsonSerializer(typeof(ColumnMapping));
                s.WriteObject(fs, this);
            }
        }
    }

    [DataContract]
    public class PropertyChangedBase : INotifyPropertyChanged   {

        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged(string name)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }
    }

    [DataContract]
    public class ColumnMap : PropertyChangedBase
    {
        [DataMember]
        public int index;
        [DataMember]
        public int pi;
        [DataMember]
        public int col;
        [DataMember]
        public int length;
        [DataMember]
        public string comment;

        public List<Led> leds;

        private double _offset;

        [DataMember]
        public double offset
        {
            get => _offset;
            set
            {
                if (_offset != value)
                {
                    _offset = value;
                    OnPropertyChanged("offset");
                }
            }
        }

        private double _ledWidth;

        public double LedWidth
        {
            get => _ledWidth;
            set
            {
                if (_ledWidth != value)
                {
                    _ledWidth = value;
                    OnPropertyChanged("LedWidth");
                }
            }
        }

    }

    public class Led : PropertyChangedBase
    {
        internal ColumnMap parent;

        internal int index;

        private Color _color = Colors.Red;
        public Color Color
        {
            get => _color;
            set
            {
                if (_color != value)
                {
                    _color = value;
                    OnPropertyChanged("Color");
                }
            }
        }

        private double _width;

        public double  Width
        {
            get => _width;
            set
            {
                if (_width != value)
                {
                    _width = value;
                    OnPropertyChanged("Width");
                }
            }
        }
    }
}
