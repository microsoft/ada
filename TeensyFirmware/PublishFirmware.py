# This firmware publishes the built firmware.hex file to an Azure blob store
# so the Server can find it there and distribute it to the Raspberry Pi Devices.

import os
import sys
import binascii
import hashlib
import hmac
from azure.storage.blob import BlobServiceClient


script_dir = os.path.dirname(os.path.abspath(__file__))
hexfile = os.path.join(script_dir, ".pio/build/teensy40/firmware.hex")
if not os.path.isfile(hexfile):
    print("Please build the firmware using 'code TeensyFirmware.code-workspace")
    sys.exit(1)


connection_string = os.getenv("ADA_STORAGE_CONNECTION_STRING")
if not connection_string:
    print("Please set ADA_STORAGE_CONNECTION_STRING environment variable")
    sys.exit(1)


def upload_file(filename, blob_container_name):
    with open(hexfile, "rb") as f:
        bytes = f.read()
        byte_key = binascii.unhexlify("414441")
        hash = hmac.new(byte_key, bytes, hashlib.sha256).hexdigest()

    print("Publish firmware with hash: {}".format(hash))

    service = BlobServiceClient.from_connection_string(conn_str=connection_string)

    container = service.get_container_client(blob_container_name)
    if not container.exists():
        container.create_container(public_access="Container")

    with open(hexfile, "rb") as data:
        container.upload_blob(filename, data, overwrite=True)

    data = bytearray(hash, 'utf-8')
    container.upload_blob(filename + ".hash", data, overwrite=True)
    print("Success")


upload_file("TeensyFirmware.TEENSY40.hex", "firmware")
