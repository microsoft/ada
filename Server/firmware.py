# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
import requests
import _thread
import time
import datetime
import os


class TeensyFirmwareUpdater:
    """ TeensyFirmwareUpdater: this class downloads new version of the firmware
    once per day, and returns the hash so that the raspberry pi devices can
    stay in sync with the latest firmware.  It assumes the given remove blob
    name is accompanies by a hash file named: blobName + ".hash". """
    def __init__(self, blob_name, filename):
        self.url = os.getenv("firmware_uri")
        self.firmware = None
        self.blob_name = blob_name
        self.filename = filename
        self.hash = None

    def start(self):
        _thread.start_new_thread(self.download_thread, ())

    def stop(self):
        self.url = None

    def download_thread(self):
        while self.url:
            hex_uri = self.url + "/" + self.blob_name
            hash_uri = hex_uri + ".hash"
            hash_file = self.filename + ".hash"

            self.download_blob(hash_uri, hash_file)
            hash = self.read_hash(hash_file)

            if self.hash != hash or not os.path.exists(self.filename):
                self.download_blob(hex_uri, self.filename)
                # must update self.hash after download_blob to ensure server
                # has the new firmware when it notices the hash update.
                self.hash = hash

            now = datetime.datetime.now()
            seconds_to_midnight = (60 - now.second)  # get us to the next minute
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

    def download_blob(self, url, filename):
        response = requests.get(url, stream=True)
        if response.status_code != 200:
            raise Exception("download file failed with status code: %d, fetching url '%s'" % (response.status_code, url))

        # Write the file to disk
        with open(filename, "wb") as handle:
            handle.write(response.content)
