// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using Azure.Storage.Blobs;
using System;
using System.IO;
using System.Text;
using System.IO.Compression;
using System.Diagnostics;

namespace AdaKioskUpdater
{
    public class AzureUpdater
    {

        public AzureUpdater()
        {
        }

        /// <summary>
        /// Returns true if the given Azure blob hash value does not match the local hash file contents.
        /// </summary>
        /// <param name="connectionString">The Azure Storage connection string</param>
        /// <param name="containerName">The Azure Storage container name</param>
        /// <param name="hashBlobName">The Azure blob name for the hash value to check</param>
        /// <param name="localHashFile">The local file where hash value is to be cached</param>
        /// <returns></returns>
        public static bool CheckForUpdate(string connectionString, string containerName, string hashBlobName, string localHashFile)
        {
            var hash = DownloadHash(connectionString, containerName, hashBlobName);
            if (File.Exists(localHashFile))
            {
                string oldHash = File.ReadAllText(localHashFile);
                if (oldHash == hash)
                {
                    // we are up to date!
                    return false;
                }
            }
            File.WriteAllText(localHashFile, hash);
            return true;
        }

        public static void InstallUpdate(string connectionString, string containerName, string zipBlobName, string localZipFile, string installFolder, string programFile)
        {
            DownloadZip(connectionString, containerName, zipBlobName, localZipFile);
            KillKioskProcess(programFile);
            ExpandZip(localZipFile, installFolder);
            StartKiosk(installFolder, programFile);
        }

        private static void StartKiosk(string installFolder, string programFile)
        {
            ProcessStartInfo info = new ProcessStartInfo();
            info.FileName = programFile;
            info.WorkingDirectory = installFolder;
            info.UserName = "Ada";
            info.UseShellExecute = true;
            Process.Start(info);
        }

        private static void KillKioskProcess(string programFile)
        {
            string processName = Path.GetFileName(programFile);
            Process[] found = Process.GetProcessesByName(processName);
            foreach (Process proc in found)
            {
                proc.Kill();
            }
        }

        public static string DownloadHash(string connectionString, string containerName, string filename)
        {
            BlobServiceClient service = new BlobServiceClient(connectionString);
            var container = service.GetBlobContainerClient(containerName);
            if (container.Exists().Value)
            {
                var client = container.GetBlobClient(filename);
                var ms = new MemoryStream();
                client.DownloadTo(ms);
                ms.Seek(0, SeekOrigin.Begin);
                var hash = new StreamReader(ms, Encoding.UTF8).ReadToEnd();
                return hash;
            }
            else
            {
                Console.WriteLine("container '{0}' not found.", containerName);
                return null;
            }
        }

        public static void DownloadZip(string connectionString, string containerName, string blobName, string filename)
        {
            BlobServiceClient service = new BlobServiceClient(connectionString);
            var container = service.GetBlobContainerClient(containerName);
            if (container.Exists().Value)
            {
                var client = container.GetBlobClient(blobName);
                using (var fs = new FileStream(filename, FileMode.Create, FileAccess.Write))
                {
                    client.DownloadTo(fs);
                }
            }
            else
            {
                Console.WriteLine("container '{0}' not found.", containerName);
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
    }
}
