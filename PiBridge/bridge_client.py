# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
import argparse
import socket
import time

_commands = ["list", "on", "off", "status"]

class PiBridgeClient:
    def __init__(self, name, client, address):
        self.name = name
        self.client = client
        self.address = address

    def send_command(self, command):
        self.client.sendall(bytes(command, 'utf-8'))
        retries = 10
        while retries > 0:
            retries -= 1
            response = str(self.client.recv(8000), "utf-8")
            if response:
                return response
        return "no response"
