# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
import argparse
import json
import socketserver
import time


class Config:
    server_port = 12345
    debug = False
    debug_commands = False


class GlobalState(object):
    colors = [[80, 0, 0], [0, 80, 0], [0, 0, 80]]
    color_index = 0
    color_time = time.time()


class ClientHandler(socketserver.StreamRequestHandler):
    def handle(self):
        cmd = str(self.rfile.readline(), "utf-8").strip()

        if Config.debug:
            print(cmd, self.client_address)

        if cmd.startswith("GET"):
            self.get_sensei(cmd)
        else:
            self.wfile.write(bytes("{'error': 'invalid command'}", "utf-8"))

    def get_sensei(self, cmd):
        # parts = cmd.split(" ")
        # target = "adapi1"
        # sequence = 0

        # if len(parts) > 1:
        #     target = parts[1]
        # if len(parts) > 2:
        #     sequence = int(parts[2])

        # cycle through the 3 colors every 4 seconds.
        index = (int)((time.time() - GlobalState.color_time) / 4) % 3
        command = {
            "command": "CrossFade",
            "colors": [GlobalState.colors[index]],
            "seconds": 4,
        }
        # {"command": "StartRain", "size": 12, "amount": 40.0 }

        json_string = json.dumps(command)
        if Config.debug_commands:
            print(json_string)
        self.wfile.write(bytes(json_string, "utf-8"))


def _main(ip_address):
    endpoint = (ip_address, Config.server_port)
    try:
        server = socketserver.TCPServer(endpoint, ClientHandler)
        server.serve_forever()
    except Exception as e:
        print(e)


if __name__ == "__main__":
    parser = argparse.ArgumentParser("ada_server makes Sensei database available to the ada raspberry pi devices")
    parser.add_argument(
        "--ip",
        help="optional IP address of the server (default 'localhost')",
        default="localhost",
    )
    args = parser.parse_args()
    _main(args.ip)
