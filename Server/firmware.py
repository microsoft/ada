import requests
import _thread
import time
import datetime
import os

# TeensyFirmwareUpdater: this class downloads new version of the firmware
# once per day, and returns the hash so that the raspberry pi devices can
# stay in sync with the latest firmware.
class TeensyFirmwareUpdater:

    def __init__(self, filename):
        self.url = os.getenv("firmware_uri")
        self.firmware = None
        self.filename = filename
        self.hash = None

    def start(self):
        _thread.start_new_thread(self.download_thread, ())

    def stop():
        self.url = None

    def download_thread(self):
        while self.url:
            self.download_firmware(self.url, self.filename)
            self.hash = self.firmware_hash(self.filename)
            now = datetime.datetime.now()
            seconds_to_midnight = (60 - now.second)  # get us to the next minute
            seconds_to_midnight += (60 - (now.minute + 1)) * 60  # gets us to the next hour
            seconds_to_midnight += (24 - (now.hour + 1)) * 3600  # gets us to the next midnight
            time.sleep(seconds_to_midnight)  # sleep to the next midnight and check firmware again then.

    def get_hash(self):
        return self.hash

    def get_firmware(self):
        with open(self.filename, "rb") as f:
            bytes = f.read()
            return bytes

    def download_firmware(self, url, filename):
        response = requests.get(url, stream=True)
        if response.status_code != 200:
            raise Exception("download file failed with status code: %d, fetching url '%s'" % (response.status_code, url))

        # Write the file to disk
        with open(filename, "wb") as handle:
            handle.write(response.content)

    def firmware_hash(self, filename):
        import hashlib
        import binascii
        import hmac
        with open(filename, "rb") as f:
            bytes = f.read()
            byte_key = binascii.unhexlify("414441")
            return hmac.new(byte_key, bytes, hashlib.sha256).hexdigest()
