# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
import numpy as np
import time


class KasaBridgeClient:
    _commands = ["list", "on", "off", "status"]

    def __init__(self, name, client, address, ping_delay=600):
        self.name = name
        self.client = client  # a socket client.
        self.address = address
        self.lights_on = None
        self.bridge_error = None
        self.last_ping = 0
        self.ping_delay = ping_delay  # make sure socket stays alive.

    def turn_on_lights(self):
        if not self.client:
            self.bridge_error = "disconnected"
            return "disconnected"
        response = self._send_bridge_command("on")
        if response == "ok":
            self.lights_on = True
        else:
            msg = "Failed to turn on the lights: {}".format(response)
            print(msg)
            self.bridge_error = msg

    def turn_off_lights(self):
        if not self.client:
            self.bridge_error = "disconnected"
            return "disconnected"
        response = self._send_bridge_command("off")
        if response == "ok":
            self.lights_on = False
        else:
            msg = "Failed to turn off the lights: {}".format(response)
            print(msg)
            self.bridge_error = msg

    def get_switch_status(self):
        result = self._send_bridge_command("status")
        if result != "error":
            parts = result.split(",")
            states = []
            for device in parts:
                pair = device.split(":")
                if len(pair) == 2:
                    is_on = pair[1] == "True"
                    states += [is_on]
            if np.all(states):
                self.lights_on = True
            elif np.all([not x for x in states]):
                self.lights_on = False
            else:
                # mixed states needs to be corrected later
                self.lights_on = None
        return result

    def update_switch_status(self):
        if not self.client:
            self.lights_on = None  # we don't know yet
            return

        # Throttle this call so that the lighting designer
        # can call it in the animation loop
        if self.last_ping + self.ping_delay > time.time():
            return

        result = self.get_switch_status()
        self.last_ping = time.time()
        return result

    def _send_command(self, command):
        if self.client:
            self.client.sendall(bytes(command, "utf-8"))
            retries = 10
            while retries > 0 and self.client:
                retries -= 1
                response = str(self.client.recv(16000), "utf-8")
                if response:
                    return response
        return "no response"

    def _send_bridge_command(self, command):
        try:
            self.bridge_error = None
            return self._send_command(command)
        except Exception as e:
            self.client = None
            self.bridge_error = str(e)
            return "error"
