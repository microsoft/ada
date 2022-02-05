using System;
using System.Linq;
using System.Diagnostics;
using System.IO;
using System.Net.NetworkInformation;
using System.Text;
using System.Net.Sockets;

namespace AdaKioskUpdater
{
    class Program
    {
        static int Main(string[] args)
        {            
            var connectionString = Environment.GetEnvironmentVariable("ADA_STORAGE_CONNECTION_STRING");
            if (string.IsNullOrEmpty(connectionString))
            {
                Console.WriteLine("Missing ADA_STORAGE_CONNECTION_STRING");
                return 1;
            }

            const string containerName = "adakiosk";
            const string updater = "c:\\temp\\AdaKioskUpdater";
            const string kioskPath = "c:\\temp\\AdaKiosk";
            if (!Directory.Exists(updater))
            {
                Directory.CreateDirectory(updater);
            }
            string zipFile = Path.Combine(updater, "AdaKiosk.zip");
            const string blobName = "AdaKiosk.zip";
            const string hashFile = blobName + ".hash";
            var hash = Updater.DownloadHash(connectionString, containerName, hashFile);
            var cached = zipFile + ".hash";
            var update = !Directory.Exists(kioskPath);
            if (!File.Exists(cached))
            {
                update = true;
            }
            else
            {
                using (var stream = new StreamReader(cached))
                {
                    var existing = stream.ReadToEnd();
                    if (existing != hash)
                    {
                        update = true;
                    }
                    else
                    {
                        Console.WriteLine("Program is up to date");
                    }
                }
            }
            if (update) {
                Console.WriteLine("Found new hash: " + hash);
                using (var sw = new StreamWriter(cached, false, Encoding.UTF8))
                {
                    sw.Write(hash);
                }

                Updater.DownloadZip(connectionString, containerName, blobName, zipFile);
                Console.WriteLine("Downloaded zip file: {0}", zipFile);

                Console.WriteLine("Expanding zip file...");
                Updater.ExpandZip(zipFile, kioskPath);
            }

            ProcessStartInfo si = new ProcessStartInfo();
            si.FileName = Path.Combine(kioskPath, "AdaKiosk.exe");
            si.WorkingDirectory = kioskPath;
            Process.Start(si);
            return 0;
        }

    }
}
