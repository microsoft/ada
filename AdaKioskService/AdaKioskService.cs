using AdaKioskUpdater;
using System;
using System.Diagnostics;
using System.IO;
using System.Management;
using System.Runtime.InteropServices;
using System.ServiceProcess;
using System.Threading;

namespace AdaKioskService
{
    public partial class AdaKioskService : ServiceBase
    {
        Timer timer;
        ServiceLog log = new ServiceLog("c:\\AdaKiosk\\adakioskservice.log");
        string connectionString;
        const string containerName = "adakiosk";
        const string installDir = "c:\\AdaKiosk\\App";
        const string downloadDir = "c:\\AdaKiosk\\Download";
        const string programName = "AdaKiosk.exe";
        bool checkForUpdates = true;

        public AdaKioskService()
        {
            InitializeComponent();
        }

        protected override void OnStart(string[] args)
        {
            log.WriteMessage("AdaKioskService started");

            this.connectionString = Environment.GetEnvironmentVariable("ADA_STORAGE_CONNECTION_STRING");
            if (string.IsNullOrEmpty(this.connectionString))
            {
                log.WriteMessage("Missing ADA_STORAGE_CONNECTION_STRING");
            }
            else
            {
                StartTimer();
            }
        }

        void StartTimer()
        {
            if (this.timer == null)
            {
                // do first check immediately, then check again every hour.
                this.timer = new Timer(OnCheckForUpdate, null, 1000, 60 * 60 * 1000);
            }
        }

        void StopTimer()
        {
            if (this.timer != null)
            {
                this.timer.Dispose();
                this.timer = null;
            }
        }

        protected override void OnStop()
        {
            log.WriteMessage("AdaKioskService stopped");
            StopTimer();
        }

        private void OnCheckForUpdate(object state)
        {
            if (!checkForUpdates)
            {
                return;
            }
            try
            {
                //var updater = new WindowsUpdater();
                //_ = updater.UpdateAndRestart(new CancellationTokenSource());
                StopTimer();
                log.WriteMessage("AdaKioskService checking for update");
                var hashFileName = "AdaKiosk.zip.hash";
                if (!Directory.Exists(downloadDir))
                {
                    Directory.CreateDirectory(downloadDir);
                }
                if (!Directory.Exists(installDir))
                {
                    Directory.CreateDirectory(installDir);
                }
                var hashFile = System.IO.Path.Combine(downloadDir, hashFileName);
                var hash = AzureUpdater.CheckForUpdate(this.connectionString, containerName, hashFileName, hashFile);
                if (hash != null)
                {
                    log.WriteMessage("AdaKioskService found update " + hash);
                    var zipFileName = "AdaKiosk.zip";
                    var zipFile = System.IO.Path.Combine(downloadDir, zipFileName);
                    AzureUpdater.InstallUpdate(this.connectionString, containerName, zipFileName, zipFile, installDir, programName);

                    log.WriteMessage("AdaKioskService bits installed to: " + installDir);
                    var exe = System.IO.Path.Combine(installDir, programName);

                    log.WriteMessage("AdaKioskService rebooting");
                    RestartComputerWmi();
                } 
                else
                {

                    log.WriteMessage("AdaKioskService is up to date");
                }
            }
            catch (Exception ex)
            {
                log.WriteMessage("AdaKioskService update error: " + ex.ToString());
            }
            StartTimer();
        }

        public void RestartComputerWmi()
        {
            ManagementObjectSearcher mos = new ManagementObjectSearcher("SELECT * FROM Win32_OperatingSystem");
            ManagementObjectCollection moc = mos.Get();
            foreach (ManagementObject mo in moc)
            {
                mo.InvokeMethod("Reboot", new string[] { "Ada Kiosk Update" });
            }
        }
    }
}
