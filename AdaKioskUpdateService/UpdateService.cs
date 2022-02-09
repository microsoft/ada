// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.IO;
using System.ServiceProcess;
using System.Threading;

namespace AdaKioskUpdateService
{
    public partial class UpdateService : ServiceBase
    {
        Timer timer;
        const int UpdatePingDelay = 15 * 60 * 1000;  // check for an update every 60 minutes

        public UpdateService()
        {
            InitializeComponent();
        }

        protected override void OnStart(string[] args)
        {
            WriteToFile("Service is started");

            timer = new Timer(new TimerCallback(OnTimerElapsed), null, UpdatePingDelay, UpdatePingDelay);
        }

        private void OnTimerElapsed(object state)
        {
        }

        protected override void OnStop()
        {
            WriteToFile("Service is stopped");
        }

        public void WriteToFile(string Message)
        {
            string path = AppDomain.CurrentDomain.BaseDirectory + "\\Logs";
            if (!Directory.Exists(path))
            {
                Directory.CreateDirectory(path);
            }
            string filepath = AppDomain.CurrentDomain.BaseDirectory + "\\Logs\\AdaKioskUpdaterLog.txt";
            if (!File.Exists(filepath))
            {
                // Create a file to write to.   
                using (StreamWriter sw = File.CreateText(filepath))
                {
                    sw.WriteLine(Message);
                }
            }
            else
            {
                using (StreamWriter sw = File.AppendText(filepath))
                {
                    sw.WriteLine(Message);
                }
            }
        }
    }
}
