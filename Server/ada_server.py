# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

import argparse
import json
import os
import time
import _thread
import socket
import sys
from threading import Lock
from collections import namedtuple
from http.server import HTTPServer
import priority_queue
import sensei
import web_server
import firmware
from signalr import MessageBus

script_dir = os.path.dirname(os.path.abspath(__file__))
sys.path += [os.path.join(script_dir, "../PiBridge")]

HttpServerPort = 4000
from bridge_client import PiBridgeClient
from lighting_designer import LightingDesigner


class AdaServer:
    """
    This class manages TCP connections to multiple remote clients including the
    raspberry pi's and the DmxController providing a "push model" interface that
    can push commands to those pi's on sockets that stay open so that we minimize
    the latency to each client.  It also provides auto-reconnection in case the
    socket is closed for any reason.
    It maintains a queue of animations we want to play with an additional concept
    of priority so something can jump to the front of the queue.
    """
    def __init__(self, config, msgbus, server_endpoint, web_address):
        self.config = config
        self.msgbus = msgbus
        self.web_address = web_address
        self.server_endpoint = server_endpoint
        self.client_queues = {}  # the pending command queue for each rpi client.
        self.clients = {}  # the dictionary of connected client sockets
        self.names = {}  # map of client address => name
        self.closed = False
        self.camera_queue = priority_queue.PriorityQueue()
        self.sequence_numbers = {}  # the client sequence numbers we are getting back
        self.sequence_numbers_sent = {}  # the client sequence numbers we have sent
        self.sequence = 0
        self.lock = Lock()
        self.camera = True  # switches camera on and off
        self.queued = False
        self.bridge = None
        self.max_animations_per_iteration = 5
        self.firmware = firmware.TeensyFirmwareUpdater("firmware.hex")

    def start(self):
        self.firmware.start()
        self.closed = False
        _thread.start_new_thread(self.serve_forever, ())
        if self.web_address:
            _thread.start_new_thread(self.web_server_thread, ())

    def web_server_thread(self):
        host_name = self.web_address
        http_server = HTTPServer((host_name, HttpServerPort), web_server.AdaWebServer)
        print("Server started http://%s:%s" % (host_name, HttpServerPort))

        try:
            http_server.serve_forever()
        except KeyboardInterrupt:
            pass

        http_server.server_close()
        print("Web Server stopped.")

    def close(self):
        self.firmware.stop()
        self.closed = True

    def get_clients(self):
        self.lock.acquire()
        result = list(self.names.values())
        self.lock.release()
        return result

    def idle(self):
        is_idle = True
        self.lock.acquire()
        for q in self.client_queues.values():
            if q.size() > 0:
                is_idle = False
                break
        self.lock.release()
        return is_idle

    def camera_off(self):
        if self.camera:
            print("### Turning camera off")
            self.camera = False

    def camera_on(self):
        if not self.camera:
            print("### Turning camera on")
            self.camera = True

    def has_clients(self):
        self.lock.acquire()
        result = len(self.names)
        self.lock.release()
        return result

    def get_stale_clients(self):
        """ return list of client names that are behind this sequence number """
        self.lock.acquire()
        result = []
        for name in self.sequence_numbers:
            x = self.sequence_numbers[name]
            y = self.sequence_numbers_sent[name]
            if abs(x - y) > self.max_animations_per_iteration and x > 0:
                print("### found stale client: {}, seq# {} != {}".format(name, x, y))
                result += [name]
        self.lock.release()
        return result

    def get_web_server_context(self):
        return web_server.Context

    def read_camera(self, timeout):
        while self.camera_queue.size() > 2:
            self.camera_queue.dequeue()  # flush the queue so we keep up to date with latest info
        return self.camera_queue.dequeue()

    def clear_queue(self):
        while self.camera_queue.size() > 0:
            self.camera_queue.dequeue()  # flush the queue so we keep up to date with latest info
        self.lock.acquire()
        for c in self.client_queues:
            queue = self.client_queues[c]
            while queue.size() > 0:
                queue.dequeue()
        self.lock.release()

    def find_client_address(self, name):
        result = None
        self.lock.acquire()
        for key in self.names:
            if self.names[key] == name:
                result = key
        self.lock.release()
        return result

    def has_client_address(self, address):
        result = False
        self.lock.acquire()
        if address in self.client_queues:
            result = True
        self.lock.release()
        return result

    def increment(self):
        """
        Increment the sequence number if we previously have queued commands.
        The sequence number represents the last batch of commands that happened in the previous
        iteration of the LightingDesigner _choreographer_thread.
        """
        if self.queued:
            self.queued = False
            self.sequence += 1

    def queue_command(self, priority, cmd):
        target = None
        self.msgbus.send('server', cmd)
        self.queued = True
        if type(cmd) == list:
            for c in cmd:
                c["sequence"] = self.sequence
                if "target" in c:
                    name = c["target"]
                    if target is not None and target != name:
                        raise Exception("Please don't queue commands to different targets in one call, thanks.")
                    target = name
        else:
            cmd["sequence"] = self.sequence
            if "target" in cmd:
                target = cmd["target"]

        if target is not None:
            address = self.find_client_address(target)
            if address is not None:
                # this is a targetted command, so send only to that client
                queue = self.client_queues[address]
                queue.enqueue(priority, cmd)
                # assume client got the command, ping will correct us otherwise
                self.lock.acquire()
                self.sequence_numbers_sent[target] = self.sequence
                self.sequence_numbers[target] = self.sequence
                self.lock.release()
            else:
                print("### dropping command to '{}' because this client is not connected".format(target))
        else:
            # this is a broadcast to all clients.
            for c in self.client_queues:
                queue = self.client_queues[c]
                queue.enqueue(priority, cmd)
                name = self.names[c]
                # assume client got the command, ping will correct us otherwise
                self.lock.acquire()
                self.sequence_numbers[name] = self.sequence
                self.sequence_numbers_sent[name] = self.sequence
                self.lock.release()

    def serve_forever(self):
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.bind(self.server_endpoint)
            s.listen()
            self.closed = False
            while not self.closed:
                client, address = s.accept()
                _thread.start_new_thread(self.client_thread, (client, address))
            s.close()
        except Exception as e:
            print("## server terminating with exception: {}".format(e))

    def client_thread(self, client, address):
        """ a new thread is created to handle the heartbeats and command sending to each connected pi"""
        name = client.recv(16000)
        while len(name) == 0:
            time.sleep(0.1)
            name = client.recv(16000)
        name = name.decode('utf-8').rstrip("\0")
        if name == "IPCAMERA":
            self.handle_camera(client, address, name)
        if name == "HS105Switches":
            self.handle_switches(client, address, name)
        else:
            self.handle_client(client, address, name)

    def handle_client(self, client, address, name):
        print("Client connected from {}: {}".format(address, name))
        if name in self.clients:
            try:
                # we have a new socket for this client, which means we need to close the old one.
                self.lock.acquire()
                if name in self.clients:
                    self.clients[name].close()
                    del self.clients[name]
                self.lock.release()
                # and wait for the other thread to terminate
                while self.has_client_address(address):
                    print("waiting for previous {} thread to terminate...".format(name))
                    time.sleep(1)
            except:
                pass

        self.lock.acquire()
        client_queue = priority_queue.PriorityQueue()  # initial queue of commands.
        self.clients[name] = client
        self.client_queues[address] = client_queue
        self.names[address] = name
        self.sequence_numbers[name] = self.max_animations_per_iteration * -2
        self.sequence_numbers_sent[name] = 0  # ensure they start out of sync to guarantee sync up
        self.lock.release()

        firmware_hash = None
        ping_time = time.time() + 1
        current_priority = 50
        next_command_time = time.time()
        while not self.closed and name in self.clients:

            hash = self.firmware.get_hash()
            if hash != firmware_hash:
                firmware_hash = hash
                if hash:
                    client_queue.enqueue(0, [{"command": "FirmwareHash", "hash": hash}])

            command = None
            result = client_queue.peek()
            if result is not None:
                priority, command = result
                # if the new command is not any higher priority and we are not done
                # with the previous command yet, then sleep (highest priority is the
                # lowest number).
                if priority >= current_priority and time.time() < next_command_time:
                    time.sleep(0.1)
                    continue

                # time to queue the new command then and pop it off the queue.
                current_priority = priority
                client_queue.dequeue()  # pop the queue

                # now let a nice smooth animation complete before moving on to next step
                seconds = 1
                if "hold" in command:
                    seconds = command["hold"]
                elif "seconds" in command:
                    seconds = command["seconds"]
                print("==== next command for {} in {} seconds".format(name, seconds))
                next_command_time = time.time() + seconds

                while client_queue.size() > 5:
                    # queue is getting too long, must be a busy day!  Time for some pruning.
                    client_queue.dequeue()
            elif time.time() < ping_time:
                # nothing to do, so just sleep.
                time.sleep(0.1)
                continue

            try:
                if command is not None:
                    # time to send the command!
                    text = json.dumps(command)
                    retries = 3
                    while retries > 0:
                        client.sendall(bytes(text, 'utf-8'))
                        data = client.recv(16000)
                        msg = data.decode('utf-8').rstrip("\0")
                        if msg == "update":
                            # pi client is requesting firmware update.
                            hex = self.firmware.get_firmware()
                            # first send 4 byte header so client know how many bytes to read.
                            number = len(hex)
                            prefix = number.to_bytes(4, byteorder='big')
                            msg = bytearray(prefix)
                            msg.extend(hex)
                            client.sendall(msg)
                            data = client.recv(16000)
                            msg = data.decode('utf-8').rstrip("\0")

                        if msg != "ok":
                            print("Unexpected response from {}: {}".format(name, msg))
                            print("Trying again {}:".format(retries))
                            time.sleep(1)
                            retries -= 1
                        else:
                            break
                else:
                    # heart beat once a second.
                    cmd = "{\"command\": \"ping\"}"
                    client.sendall(bytes(cmd, 'utf-8'))
                    data = client.recv(16000)
                    msg = data.decode('utf-8').rstrip("\0").strip()
                    # response to ping is last client 'sequence' number.
                    if msg:
                        try:
                            msgno = int(msg)
                            changed = False
                            self.lock.acquire()
                            if self.sequence_numbers[name] != msgno:
                                self.sequence_numbers[name] = msgno
                                changed = True
                            self.lock.release()
                            if changed:
                                print("Client {} returned ping response {} and current sequence is {}".format(
                                    name, msgno, self.sequence))
                        except:
                            print("Unexpected ping response from {}: {}".format(name, msg))
                            pass
                    ping_time = time.time() + 1

            except socket.error as e:
                client.close()
                print("### socket error with pi {}: {}".format(name, e))
                break

        self.lock.acquire()
        if address in self.client_queues:
            del self.client_queues[address]
        if name in self.sequence_numbers:
            del self.sequence_numbers[name]
        if name in self.sequence_numbers_sent:
            del self.sequence_numbers_sent[name]
        if address in self.names:
            del self.names[address]
        if name in self.clients:
            del self.clients[name]
        self.lock.release()

    def handle_camera(self, client, address, name):
        print("Camera connected from {}: {}".format(address, name))
        # the camera protocol is reversed so camera can push updates to us any time.
        # including a heartbeat ping every so often to keep the socket alive.
        while not self.closed:
            try:
                msg = "{}".format(self.camera)
                client.sendall(bytes(msg, 'utf-8'))
                data = client.recv(16000)
                while len(data) == 0 and not self.closed:
                    data = client.recv(16000)

                if not self.closed:
                    msg = data.decode('utf-8')
                    if msg != "ping":
                        print("### Camera: received " + msg)
                        self.camera_queue.enqueue(0, msg)

            except socket.error as e:
                client.close()
                print("### socket error with camera {}: {}".format(name, e))
                break

    def handle_switches(self, client, address, name):
        print("HS105 bridge connected from {}: {}".format(address, name))
        self.bridge = PiBridgeClient(name, client, address)
        try:
            self.bridge.send_command("connected")
        except:
            self.bridge = None

    def get_bridge(self):
        return self.bridge

    def on_bridge_error(self):
        # assume it's gone then until we get another HS105Switches call.
        print("HS105 bridge is disconnected")
        self.bridge = None


def _main(config, sensei, ip_address, web_address):
    endpoint = (ip_address, config.server_port)
    msgbus = None
    signalr_uri = os.getenv("signalr_uri")
    if signalr_uri:
        msgbus = MessageBus(signalr_uri)
        msgbus.connect()
    server = AdaServer(config, msgbus, endpoint, web_address)
    server.start()
    designer = LightingDesigner(server, msgbus, sensei, config)
    designer.start()
    input("press ENTER to terminate the server\n")


if __name__ == '__main__':
    with open(os.path.join(script_dir, "config.json"), "r") as f:
        d = json.load(f)
        config = namedtuple("Config", d.keys())(*d.values())

    parser = argparse.ArgumentParser("ada_server makes Sensei database available to the ada raspberry pi devices")
    parser.add_argument("--ip", help="optional IP address of the server (default 'localhost')", default="localhost")
    parser.add_argument("--web", help="optional IP address of the web server", default=None)
    parser.add_argument("--loop", help="Loop values from a file or not (default 'false')", action="store_true")
    parser.add_argument("--delay", type=int, default=config.playback_delay,
                        help="Timeout in seconds between each row of replay loop (default {})".format(
                            config.playback_delay))
    args = parser.parse_args()

    connection_string = os.getenv("ADA_STORAGE_CONNECTION_STRING")
    if not connection_string and not args.loop:
        print("Please configure your ADA_STORAGE_CONNECTION_STRING environment variable")
        sys.exit(1)

    sensei = sensei.Sensei(config.camera_zones, config.colors_for_emotions,
                           connection_string, config.debug)
    if args.loop:
        history_files = os.path.join(os.path.join(script_dir, config.history_dir))
        sensei.load(history_files, args.delay, config.playback_weights)

    _main(config, sensei, args.ip, args.web)
