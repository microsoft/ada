using System;
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
                // don't let the log file get too big!  Reset it on each system reboot.
                File.Delete(fileName);
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
