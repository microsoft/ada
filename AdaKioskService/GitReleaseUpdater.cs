// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

using AdaKioskService;
using Microsoft.Win32;
using System;
using System.IO;
using System.IO.Compression;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Threading;
using System.Threading.Tasks;
using System.Xml.Linq;

namespace AdaKioskUpdater
{
    public static class GitReleaseUpdater
    {
        /// <summary>
        /// Check if the given github version has changed or not, returns the new version or null if no change.
        /// </summary>
        /// <param name="versionUrl">Location of Version.props file to download</param>
        /// <param name="localVersionFile">The local file containing previous version</param>
        public static async Task<Tuple<string, int>> CheckForUpdate(string versionUrl, string localVersionFile)
        {
            // https://raw.githubusercontent.com/microsoft/ada/main/AdaKiosk/Version/Version.props
            var text = await DownloadText(versionUrl);
            var xdoc = XDocument.Parse(text);
            var version = xdoc.Root.Element("PropertyGroup")?.Element("ApplicationVersion")?.Value;
            if (string.IsNullOrEmpty(version))
            {
                throw new Exception($"Could not find an Project/PropertyGroup/ApplicationVersion in the requested url: {versionUrl}");
            }

            var updateDelay = xdoc.Root.Element("PropertyGroup")?.Element("UpdateDelay")?.Value;
            int newUpdateRate = 0;
            int.TryParse(updateDelay, out newUpdateRate);

            version = version.Trim();

            if (File.Exists(localVersionFile))
            {
                string oldVersion = File.ReadAllText(localVersionFile).Trim();
                if (oldVersion == version)
                {
                    // we are up to date!
                    return new Tuple<string, int>(null, newUpdateRate);
                }
            }
            File.WriteAllText(localVersionFile, version);
            return new Tuple<string,int>(version, newUpdateRate);
        }

        static async Task<string> DownloadText(string url)
        {
            HttpClient client = new HttpClient();
            client.DefaultRequestHeaders.CacheControl = new CacheControlHeaderValue { NoCache = true };
            return await client.GetStringAsync(url);
        }


        public static async Task InstallUpdate(string url, string localZipFile, string installFolder, string processName)
        {
            await DownloadBinaryFile(url, localZipFile);
            if (ApplicationLauncher.TerminateProcessInUserSession(ServiceLog.Instance, processName, true))
            {
                Thread.Sleep(1000);
            }

            ExpandRetry(localZipFile, installFolder);
        }

        private static void ExpandRetry(string localZipFile, string installFolder)
        {
            for (int retries = 3; retries > 0; retries--)
            {
                try
                {
                    ExpandZip(localZipFile, installFolder);
                    break;
                }
                catch (Exception)
                {
                    Thread.Sleep(1000);
                }
            }
        }

        public static async Task DownloadAndUnzip(string url, string localZipFile, string installFolder)
        {
            await DownloadBinaryFile(url, localZipFile);
            ExpandRetry(localZipFile, installFolder);
        }

        static async Task DownloadBinaryFile(string url, string localFile)
        {
            HttpClient client = new HttpClient();
            client.DefaultRequestHeaders.CacheControl = new CacheControlHeaderValue { NoCache = true };
            var response = await client.GetAsync(url);
            if (response.IsSuccessStatusCode)
            {
                using (var fs = new FileStream(localFile, FileMode.Create, FileAccess.Write))
                {
                    await response.Content.CopyToAsync(fs);
                }
            }
            else
            {
                throw new Exception("Error downloading url: " + url + "\n" + response.ReasonPhrase);
            }
        }

        public static void ExpandZip(string zipFile, string folder)
        {
            if (Directory.Exists(folder))
            {
                Directory.Delete(folder, true);
            }
            ZipFile.ExtractToDirectory(zipFile, folder);
        }

        public static void SetupRebootRunAction(string script)
        {
            using (var key = Registry.LocalMachine.OpenSubKey("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce", true))
            {
                if (key != null)
                {
                    key.SetValue("AdaKioskService", script);
                }
            }
        }
    }
}
