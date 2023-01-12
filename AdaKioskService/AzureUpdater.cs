﻿// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

using System.Management;
using System.Management.Instrumentation;
using Azure.Storage.Blobs;
using System;
using System.IO;
using System.Text;
using System.IO.Compression;
using System.Diagnostics;
using AdaKioskService;
using System.Threading;
using Microsoft.Win32;

namespace AdaKioskUpdater
{
    public static class AzureUpdater
    {
        /// <summary>
        /// Check if the given Azure blob hash value does not match the local hash file contents.
        /// </summary>
        /// <param name="connectionString">The Azure Storage connection string</param>
        /// <param name="containerName">The Azure Storage container name</param>
        /// <param name="hashBlobName">The Azure blob name for the hash value to check</param>
        /// <param name="localHashFile">The local file where hash value is to be cached</param>
        /// <returns>A new hash value if one is available, or null if your hash is up to date</returns>
        public static string CheckForUpdate(string connectionString, string containerName, string hashBlobName, string localHashFile)
        {
            var hash = DownloadHash(connectionString, containerName, hashBlobName);
            if (File.Exists(localHashFile))
            {
                string oldHash = File.ReadAllText(localHashFile);
                if (oldHash == hash)
                {
                    // we are up to date!
                    return null;
                }
            }
            File.WriteAllText(localHashFile, hash);
            return hash;
        }


        public static void InstallUpdate(string connectionString, string containerName, string zipBlobName, string localZipFile, string installFolder, string processName)
        {
            DownloadZip(connectionString, containerName, zipBlobName, localZipFile);
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

        public static void DownloadAndUnzip(string connectionString, string containerName, string zipBlobName, string localZipFile, string installFolder)
        {
            DownloadZip(connectionString, containerName, zipBlobName, localZipFile);
            ExpandRetry(localZipFile, installFolder);
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
