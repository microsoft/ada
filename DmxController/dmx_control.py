# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
import argparse
import socket
import _thread
import queue
import json as JSON

from dmx import Dmx, DmxDevice
import dmx_mock
import time


# In RPi, use the symlinks in /dev/serial/by-id/ instead of /dev/serialX
SERVER_PORT = 12345
SOCKET_BUFFER_SIZE = 32768
DMX_UNIVERSE = 1
NUM_LIGHTS = 6
ALL_LIGHTS = ["light_1", "light_2", "light_3", "light_4", "light_5", "light_6"]


# this class manages the socket connection to the server and provides a
# queue of commands to process.
class Sensei:
    def __init__(self, server_endpoint):
        self.server_endpoint = server_endpoint
        self.running = True
        self.socket = None
        self.queue = queue.Queue()
        self.sequence = -10  # force it to be out of sync initially
        _thread.start_new_thread(self.server_thread, ())

    def __exit__(self):
        self.running = False
        if self.socket is not None:
            self.socket.close()

    def update_sequence(self, seqno):
        if self.sequence != seqno:
            print("### new sequence {}".format(seqno))
            self.sequence = seqno

    def server_thread(self):
        while self.running:
            try:
                with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
                    self.socket = sock
                    sock.connect(self.server_endpoint)
                    sock.sendall(bytes('DMX', 'utf-8'))
                    while self.running:
                        json_msg = str(sock.recv(SOCKET_BUFFER_SIZE), "utf-8")
                        try:
                            response = "ok"
                            command = JSON.loads(json_msg)
                            if "command" in command and command["command"] == "ping":
                                response = "{}".format(self.sequence)

                            sock.sendall(bytes(response, 'utf-8'))

                            self.queue.put(command)
                        except Exception as e:
                            msg = "### Exception: {}".format(e)
                            sock.sendall(bytes(msg, 'utf-8'))
            except:
                time.sleep(1)

    def get_next_command(self, timeout=1):
        try:
            return self.queue.get(timeout=timeout)
        except queue.Empty:
            return None


class DmxController:
    def __init__(self, server_endpoint, dmx):
        self.current_colors = [[0, 0, 0, 0, 0, 0]] * NUM_LIGHTS
        self.sensei = Sensei(server_endpoint)
        self.dmx = dmx
        for name in ALL_LIGHTS:
            # Each device will be assigned start DMX address 1, and will use up to address 6
            dmx.add_device(DMX_UNIVERSE, DmxDevice(name, "power_lights", NUM_LIGHTS, 1))

        # Value mapping is: [red, green, blue, white, amber, uv]
        dmx.set_output_by_type(DMX_UNIVERSE, "power_lights", [255, 220, 0, 0, 0, 0])

    def handle_command(self, command):

        if "sequence" in command:
            self.sensei.update_sequence(int(command["sequence"]))

        if command["command"] == "ping":
            return
        if "colors" in command:
            colors = command["colors"]
            new_colors = [(x[0], x[1], x[2]) for x in colors]
        else:
            print("### ignoring command : {}".format(command))
            return

        for i in range(0, len(new_colors)):
            if len(new_colors[i]) < NUM_LIGHTS:
                for j in range(0, NUM_LIGHTS - len(new_colors[i])):
                    new_colors[i] += (0,)

        # make sure we have at least NUM_LIGHTS colors.
        while len(new_colors) < NUM_LIGHTS:
            new_colors += [new_colors[0]]

        if args.color == "UV":
            new_colors = [[0, 0, 0, 0, 0, 255]] * NUM_LIGHTS
        # 2 second fade to new color
        print("seq {}: {}".format(self.sensei.sequence, new_colors))
        dmx.smooth_fade(DMX_UNIVERSE, ALL_LIGHTS, self.current_colors, new_colors, 2)
        self.current_colors = new_colors

    def run_program(self):

        while True:
            try:
                command = self.sensei.get_next_command()
                if command is None:
                    continue

                if type(command) is list:
                    for cmd in command:
                        self.handle_command(cmd)
                else:
                    self.handle_command(command)

            except Exception as e:
                print(e)
                # if server is offline then fade to black for now to save power.
                new_colors = [[0, 0, 0, 0, 0, 0]] * NUM_LIGHTS
                dmx.smooth_fade(DMX_UNIVERSE, ALL_LIGHTS, self.current_colors, new_colors, 10)
                self.current_colors = new_colors


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description='Pulls Sensei data from the ada_server for display on the raspberry pi')
    parser.add_argument("--ip", help="optional IP address of the server (default 'localhost')", default="localhost")
    parser.add_argument("--color", help="Specify a specific color, including 'UV' for UV light.", default="none")
    parser.add_argument("--test", help="Do not actually connect to DMX lights.", action="store_true")
    parser.add_argument("--port", help="Serial port to use (default COM3).", default="com3")
    args = parser.parse_args()

    server_endpoint = (args.ip, SERVER_PORT)
    if args.test:
        dmx = dmx_mock.Dmx(args.port)
    else:
        dmx = Dmx(args.port)

    controller = DmxController(server_endpoint, dmx)
    controller.run_program()
