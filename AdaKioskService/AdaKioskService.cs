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
        const string serviceTempDir = "c:\\AdaKiosk\\ServiceTemp";
        const string programName = "AdaKiosk.exe";
        bool checkForUpdates = true;
        const int UpdateCheckDelay = 15 * 60 * 1000; // every 15 minutes.

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
                this.timer = new Timer(OnCheckForUpdate, null, UpdateCheckDelay, UpdateCheckDelay);
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
                StopTimer();
                log.WriteMessage("AdaKioskService checking for update");
                if (!Directory.Exists(downloadDir))
                {
                    Directory.CreateDirectory(downloadDir);
                }
                if (!Directory.Exists(installDir))
                {
                    Directory.CreateDirectory(installDir);
                }
                if (!Directory.Exists(serviceTempDir))
                {
                    Directory.CreateDirectory(serviceTempDir);
                }

                // Check if there are new service bits.
                var hashFileName = "AdaKioskService.zip.hash";
                var hashFile = System.IO.Path.Combine(downloadDir, hashFileName);
                var hash = AzureUpdater.CheckForUpdate(this.connectionString, containerName, hashFileName, hashFile);
                var updated = false;
                if (hash != null)
                {
                    log.WriteMessage("AdaKioskService found service update {0}", hash);
                    var zipFileName = "AdaKioskService.zip";
                    var zipFile = System.IO.Path.Combine(downloadDir, zipFileName);
                    AzureUpdater.DownloadAndUnzip(this.connectionString, containerName, zipFileName, zipFile, serviceTempDir);

                    log.WriteMessage("AdaKioskService new bits installed to: {0}", serviceTempDir);
                    var updateScript = System.IO.Path.Combine(serviceTempDir, "Setup", "InstallService.ps1");
                    AzureUpdater.SetupRebootRunAction($"c:\\WINDOWS\\WindowsPowerShell\\v1.0\\powershell.exe -f {updateScript}");

                    updated = true;
                }


                // Now check for updated AdaKiosk app
                hashFileName = "AdaKiosk.zip.hash";
                hashFile = System.IO.Path.Combine(downloadDir, hashFileName);
                hash = AzureUpdater.CheckForUpdate(this.connectionString, containerName, hashFileName, hashFile);
                if (hash != null)
                {
                    log.WriteMessage("AdaKioskService found app update " + hash);
                    var zipFileName = "AdaKiosk.zip";
                    var zipFile = System.IO.Path.Combine(downloadDir, zipFileName);
                    AzureUpdater.InstallUpdate(this.connectionString, containerName, zipFileName, zipFile, installDir, programName);

                    log.WriteMessage("AdaKiosk new bits installed to: " + installDir);
                    var exe = System.IO.Path.Combine(installDir, programName);

                    updated = true;
                }

                if (updated) { 
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
