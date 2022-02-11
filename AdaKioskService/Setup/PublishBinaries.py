# This script publishes a zip file containing the release build of this
# AppKiosk binaries to the Azure Blob store at ADA_STORAGE_CONNECTION_STRING.

import os
import sys
import binascii
import hashlib
import hmac
import zipfile
from azure.storage.blob import BlobServiceClient
from shutil import rmtree

connection_string = os.getenv("ADA_STORAGE_CONNECTION_STRING")
if not connection_string:
    print("Please set ADA_STORAGE_CONNECTION_STRING environment variable")
    sys.exit(1)


zip_file_name = 'AdaKiosk.zip'
blob_store_container = 'adakiosk'

script_dir = os.path.dirname(os.path.abspath(__file__))
folder = os.path.join(script_dir, "..", "..", "AdaKiosk", "bin", "Release", "net5.0-windows")


def zipdir(path, ziph):
    count = 0
    # ziph is zipfile handle
    for root, dirs, files in os.walk(path):
        for file in files:
            fullPath = os.path.join(root, file)
            relative_path = fullPath[len(path) + 1:]
            print("zipping: " + relative_path)
            ziph.write(fullPath, relative_path)
            count += 1
    return count


def create_zip(filename, folder):
    print("Creating zip file: " + filename)
    if os.path.exists(filename):
        os.remove(filename)
    with zipfile.ZipFile(filename, 'w', zipfile.ZIP_DEFLATED) as zipf:
        return zipdir(folder, zipf)


def upload_file(filename, container):

    with open(filename, "rb") as f:
        bytes = f.read()
        byte_key = binascii.unhexlify("414441")
        hash = hmac.new(byte_key, bytes, hashlib.sha256).hexdigest()
    print(f"Uploading {filename} with hash: {hash}")

    service = BlobServiceClient.from_connection_string(conn_str=connection_string)

    container = service.get_container_client(blob_store_container)
    if not container.exists():
        container.create_container(public_access="Container")

    with open(filename, "rb") as data:
        container.upload_blob(filename, data, overwrite=True, validate_content=True)

    hashData = bytearray(hash, 'utf-8')
    container.upload_blob(filename + ".hash", hashData, overwrite=True)


if __name__ == '__main__':
    if not os.path.exists(folder):
        print("Please build the Release build.")
        sys.exit(1)

    # remove webview 2 files created when debugging the app locally.
    webview2tempfiles = os.path.join(folder, "AdaKiosk.exe.WebView2")
    if os.path.exists(webview2tempfiles):
        rmtree(webview2tempfiles)

    # remove any msix publishing stuff.
    win_x86 = os.path.join(folder, "win-x86")
    if os.path.exists(win_x86):
        rmtree(win_x86)

    count = create_zip(zip_file_name, folder)
    if count > 40:
        print("Found too many files in the bin folder.  Does it contain some junk?")
        sys.exit(1)

    upload_file(zip_file_name, blob_store_container)
    os.remove(zip_file_name)
