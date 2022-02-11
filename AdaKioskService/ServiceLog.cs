using System;
using System.Collections.Generic;
using System.IO;
using System.Threading;

namespace AdaKioskService
{
    class ServiceLog
    {
        string fileName;
        static ServiceLog _instance;

        public ServiceLog(string fileName)
        {
            _instance = this;
            this.fileName = fileName;            
            if (File.Exists(fileName))
            {
                // don't let the log file get too big!  Truncate it to the last 1000 lines.
                var lines = new List<string>(File.ReadAllLines(fileName));
                if (lines.Count > 1000)
                {
                    lines.RemoveRange(0, lines.Count - 1000);
                    File.WriteAllLines(fileName, lines.ToArray());
                }
            }
        }

        public static ServiceLog Instance => _instance;

        public void WriteMessage(string format, params object[] args)
        {
            var msg = DateTime.Now.ToString("g") + ": " + string.Format(format, args) + "\n";
            File.AppendAllText(this.fileName, msg);
        }
    }
}
