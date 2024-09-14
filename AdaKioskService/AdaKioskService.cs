using AdaKioskUpdater;
using System;
using System.Diagnostics;
using System.IO;
using System.Management;
using System.Runtime.InteropServices;
using System.ServiceProcess;
using System.Threading;
using System.Threading.Tasks;

namespace AdaKioskService
{
    public partial class AdaKioskService : ServiceBase
    {
        Timer timer;
        ServiceLog log = new ServiceLog("c:\\AdaKiosk\\adakioskservice.log");
        string gitBaseUrl;
        const string mainBranch = "main";
        const string installDir = "c:\\AdaKiosk\\App";
        const string downloadDir = "c:\\AdaKiosk\\Download";
        const string serviceTempDir = "c:\\AdaKiosk\\ServiceTemp";
        const string programName = "AdaKiosk.exe";
        bool checkForUpdates = true;
        internal int UpdateCheckDelay = 15 * 60 * 1000; // every 15 minutes.

        public AdaKioskService()
        {
            InitializeComponent();
            System.IO.Directory.CreateDirectory("c:\\AdaKiosk");
        }

        internal void DebugStart()
        {
            OnStart(new string[0]);
        }

        protected override void OnStart(string[] args)
        {
            log.WriteMessage("AdaKioskService started");

            this.gitBaseUrl = Environment.GetEnvironmentVariable("ADA_GIT_REPO");
            if (string.IsNullOrEmpty(this.gitBaseUrl))
            {
                log.WriteMessage("Missing ADA_GIT_REPO");
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
            StopTimer();
            OnCheckForUpdateAsync().Wait();
        }

        private async Task OnCheckForUpdateAsync()
        {
            if (!checkForUpdates)
            {
                return;
            }
            try
            {
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
                var hashFileName = "AdaKioskService.zip.version";
                var hashFile = System.IO.Path.Combine(downloadDir, hashFileName);
                var uri = new Uri(this.gitBaseUrl);
                var path = uri.PathAndQuery;
                var githubRawFiles = "https://raw.githubusercontent.com/";
                var serviceVersionUrl = $"{githubRawFiles}/{path}/{mainBranch}/AdaKioskService/Version/Version.props";
                var newVersion = await GitReleaseUpdater.CheckForUpdate(serviceVersionUrl, hashFile);
                var updated = false;
                if (newVersion != null)
                {
                    // var url = $"{baseReleasesUrl}/{version}/{zipFileName}";
                    var zipFileName = "AdaKioskService.zip";
                    var zipUrl = $"{this.gitBaseUrl}/releases/{newVersion}/{zipFileName}";
                    log.WriteMessage("AdaKioskService found service version {0}", newVersion);
                    var zipFile = System.IO.Path.Combine(downloadDir, zipFileName);
                    await GitReleaseUpdater.DownloadAndUnzip(zipUrl, zipFile, serviceTempDir);

                    log.WriteMessage("AdaKioskService new bits installed to: {0}", serviceTempDir);
                    var updateScript = System.IO.Path.Combine(serviceTempDir, "Setup", "InstallService.ps1");
                    GitReleaseUpdater.SetupRebootRunAction($"c:\\WINDOWS\\WindowsPowerShell\\v1.0\\powershell.exe -f {updateScript}");
                    updated = true;
                }


                // Now check for updated AdaKiosk app
                hashFileName = "AdaKiosk.zip.version";
                hashFile = System.IO.Path.Combine(downloadDir, hashFileName);
                var kioskVersionUrl = $"{githubRawFiles}/{path}/{mainBranch}/AdaKiosk/Version/Version.props";
                newVersion = await GitReleaseUpdater.CheckForUpdate(kioskVersionUrl, hashFile);
                if (newVersion != null)
                {
                    log.WriteMessage("AdaKioskService found app update " + newVersion);
                    var zipFileName = "AdaKiosk.zip";
                    var zipUrl = $"{this.gitBaseUrl}/releases/{newVersion}/{zipFileName}";
                    var zipFile = System.IO.Path.Combine(downloadDir, zipFileName);
                    await GitReleaseUpdater.InstallUpdate(zipUrl, zipFile, installDir, programName);

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
