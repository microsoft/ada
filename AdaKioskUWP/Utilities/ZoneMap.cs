// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System.Collections.Generic;
using System.IO;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;

namespace AdaSimulation
{
    [DataContract]
    public class ZoneMap
    {
        [DataMember(Name="zone_leds")]
        public List<StripMap> ZoneLeds;

        [DataMember(Name = "core_leds")]
        public List<StripMap> CoreLeds;

        internal string FileName;

        public void RemoveZone(int zone)
        {
            foreach (var item in ZoneLeds.ToArray())
            {
                if (item.zone == zone)
                {
                    ZoneLeds.Remove(item);
                }
            }
        }

        public static ZoneMap Load(string filename)
        {
            using (var fs = new FileStream(filename, FileMode.Open, FileAccess.Read))
            {
                DataContractJsonSerializer s = new DataContractJsonSerializer(typeof(ZoneMap));
                var result = s.ReadObject(fs) as ZoneMap;
                result.FileName = filename;
                return result;
            }
        }

        public void Save(string filename)
        {
            using (var fs = new FileStream(filename, FileMode.Create, FileAccess.Write))
            {
                DataContractJsonSerializer s = new DataContractJsonSerializer(typeof(ZoneMap));
                s.WriteObject(fs, this);
            }
        }
    }

    [DataContract]
    public class StripMap
    {
        [DataMember]
        public int col;

        [DataMember]
        public int zone = -1;
    }
}
