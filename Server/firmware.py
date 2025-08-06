# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
import _thread
import datetime
import logging
import os
import time

from azure.identity import DefaultAzureCredential
from azure.storage.blob import BlobClient

from logger import Logger

logger = Logger()
log = logger.get_root_logger()


class TeensyFirmwareUpdater:
    """TeensyFirmwareUpdater: this class downloads new version of the firmware
    once per day, and returns the hash so that the raspberry pi devices can
    stay in sync with the latest firmware.  It assumes the given remove blob
    name is accompanies by a hash file named: blobName + ".hash"."""

    def __init__(self, blob_storage_url, container_name, blob_name, filename):
        self.closed = False
        self.firmware = None
        self.container_name = container_name
        self.blob_name = blob_name
        self.filename = filename
        self.hash = None
        self.blob_storage_url = blob_storage_url

    def start(self):
        _thread.start_new_thread(self.download_thread, ())

    def stop(self):
        self.closed = True

    def download_thread(self):
        while not self.closed:
            hash_blob = self.blob_name + ".hash"
            hash_file = self.filename + ".hash"

            self.download_blob(hash_blob, hash_file)
            hash = self.read_hash(hash_file)

            if self.hash != hash or not os.path.exists(self.filename):
                self.download_blob(self.blob_name, self.filename)
                # must update self.hash after download_blob to ensure server
                # has the new firmware when it notices the hash update.
                self.hash = hash

            now = datetime.datetime.now()
            seconds_to_midnight = 60 - now.second  # get us to the next minute
            seconds_to_midnight += (60 - (now.minute + 1)) * 60  # gets us to the next hour
            seconds_to_midnight += (24 - (now.hour + 1)) * 3600  # gets us to the next midnight
            time.sleep(seconds_to_midnight)  # sleep to the next midnight and check firmware again then.

    def read_hash(self, hash_file):
        if os.path.exists(hash_file):
            with open(hash_file, "r") as f:
                return f.read().strip()
        return None

    def get_hash(self):
        return self.hash

    def get_firmware(self):
        with open(self.filename, "rb") as f:
            bytes = f.read()
            return bytes

    def download_blob(self, blob_name, filename):
        try:
            logger = logging.getLogger("azure.core.pipeline.policies.http_logging_policy")
            logger.setLevel(logging.ERROR)
            blob_client = BlobClient(
                self.blob_storage_url,
                self.container_name,
                blob_name=blob_name,
                credential=DefaultAzureCredential(),
            )

            with open(filename, "wb") as f:
                data = blob_client.download_blob()
                f.write(data.readall())

        except Exception as e:
            log.error(f"### Error downloading blob '{filename}' to local file: {e}")


# for a quick test...
# if __name__ == "__main__":
#     constr = os.getenv("ADA_STORAGE_CONNECTION_STRING")
#     up = TeensyFirmwareUpdater("firmware", "TeensyFirmware.TEENSY40.hex", "firmware.hex", constr)
#     up.start()
#     time.sleep(60)
