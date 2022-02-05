using Azure.Storage.Blobs;
using System;
using System.IO;
using System.Text;
using System.IO.Compression;


namespace AdaKioskUpdater
{
    public class Updater
    {
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
