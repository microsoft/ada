# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
import os
import time
import socket
import buildtools


builder = buildtools.BuildTools()
os.chdir("/home/pi/git/Ada/RpiController/build")
builder.run(["git", "pull"])
builder.run("make")

while True:
    os.chdir("/home/pi/git/Ada/RpiController/build/bin")
    rc = builder.run(["/home/pi/git/Ada/RpiController/build/bin/RpiController", "--autostart"])
    if rc == 1:
        # recover or update the teensy by re-flashing it!
        builder.run("/home/pi/git/Ada/TeensyFirmware/flash.sh")
        time.sleep(5)
