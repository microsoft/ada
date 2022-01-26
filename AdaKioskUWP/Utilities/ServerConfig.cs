// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using Windows.Storage;
using Windows.UI;
using Windows.UI.Xaml;

namespace AdaSimulation
{
    public static class AppContent
    {
        public static string Root { get; internal set; }

        public static string FindContent(string name)
        {
            string path = System.IO.Path.Combine(Root, name);
            if (File.Exists(path))
            {
                return path;
            }
            return null;
        }
    }

    [DataContract]
    public class ServerConfig
    {
        [DataMember(Name = "colors_for_emotions")]
        public EmotionColors EmotionColors;

        public static ServerConfig Load(string filename)
        {
            using (var fs = new FileStream(filename, FileMode.Open, FileAccess.Read))
            {
                DataContractJsonSerializer s = new DataContractJsonSerializer(typeof(ServerConfig));
                var result = s.ReadObject(fs) as ServerConfig;
                return result;
            }
        }
    }

    [DataContract]
    public class EmotionColors
    {
        [DataMember]
        public int[] anger;
        [DataMember]
        public int[] contempt;
        [DataMember]
        public int[] disgust;
        [DataMember]
        public int[] fear;
        [DataMember]
        public int[] happiness;
        [DataMember]
        public int[] neutral;
        [DataMember]
        public int[] sadness;
        [DataMember]
        public int[] surprise;

        public Color GetColor(string name)
        {
            int[] values = null;
            switch (name)
            {
                case "anger":
                    values = this.anger;
                    break;
                case "contempt":
                    values = this.contempt;
                    break;
                case "disgust":
                    values = this.disgust;
                    break;
                case "fear":
                    values = this.fear;
                    break;
                case "happiness":
                    values = this.happiness;
                    break;
                case "neutral":
                    values = this.neutral;
                    break;
                case "sadness":
                    values = this.sadness;
                    break;
                case "surprise":
                    values = this.surprise;
                    break;
                default:
                    break;
            }
            if (values != null)
            {
                int r = values.Length > 0 ? values[0] : 0;
                int g = values.Length > 1 ? values[1] : 0;
                int b = values.Length > 2 ? values[2] : 0;
                return Color.FromArgb(255, (byte)r, (byte)g, (byte)b);
            }
            return Colors.Black;
        }
    }
}
