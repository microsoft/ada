# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
import argparse
import socket
import time
import _thread
from tplink_smartplug import TplinkSmartPlug

class TplinkServer:
    def __init__(self, local_ip, server_ip, server_port):
        self.local_ip = local_ip
        self.server_endpoint = (server_ip, server_port)

    def start(self, plugs):
        print("starting server on : {}".format(self.server_endpoint), flush=True)
        self.closed = False
        self.plugs = plugs
        _thread.start_new_thread(self.serve_forever, ())

    def stop(self):
        self.closed = True

    def serve_forever(self):
        while not self.closed:
            try:
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.bind((self.local_ip,0))
                s.connect(self.server_endpoint)
                s.sendall(bytes("HS105Switches", 'utf-8'))
                last_ping = time.time()
                while True:
                    request = s.recv(16000)
                    if request is not None:
                        msg = request.decode("utf-8")
                        print("Bridge received command: {}".format(msg))
                        parts = msg.split(":")
                        command = parts[0]
                        if command == "list":
                            # return list of known tplink devices
                            response = "{}".format(self.plugs)
                        elif command == "on":
                            # turn on all devices
                            response = self.turn_all_on()
                        elif command == "off":
                            # turn off all devices
                            response = self.turn_all_off()
                        elif command == "status":
                            response = self.get_status()
                        elif command == "connected":
                            print("Connected to ada server")
                            response = "ok"
                        else:
                            response = "unknown request: " + request
                        s.sendall(bytes(response, 'utf-8'))

            except Exception as e:
                print("## server terminating with exception: {}".format(e), flush=True)
                time.sleep(5)

    def turn_all_on(self):
        retries = 5
        while retries > 0:
            retries -= 1
            all_on = True
            for local_ip, switch_ip in self.plugs:
                plug = TplinkSmartPlug(local_ip, switch_ip)
                plug.get_info()
                if not plug.is_on:
                    all_on = False
                    print("Turning on {}...".format(addr), flush=True)
                    plug.turn_on()
                    time.sleep(1)  # do not switch them all at the same time
            if all_on:
                return "ok"
        else:
            return "failed"

    def turn_all_off(self):
        retries = 5
        while retries > 0:
            retries -= 1
            all_off = True
            for local_ip, switch_ip in self.plugs:
                plug = TplinkSmartPlug(local_ip, switch_ip)
                plug.get_info()
                if plug.is_on:
                    all_off = False
                    print("Turning off {}...".format(addr), flush=True)
                    plug.turn_off()
                    time.sleep(1)  # do not switch them all at the same time
            if all_off:
                return "ok"
        else:
            return "failed"

    def get_status(self):
        status = []
        for local_ip, switch_ip in self.plugs:
            plug = TplinkSmartPlug(local_ip, switch_ip)
            plug.get_info()
            print("Plug {} is {}".format(addr, plug.is_on == 1), flush=True)
            status += ["{}:{}".format(addr, plug.is_on == 1)]
        return ",".join(status)


def find_local_ips(local_ip, server_name, server_port):
    server_addresses = socket.gethostbyname_ex(server_name)[2]
    hostname = socket.gethostname()
    addresses =  socket.gethostbyname_ex(hostname + ".local")[2]
    if local_ip:
        addresses = [local_ip]
        if server_name.startswith("127.") or server_name == "localhost":
            return [(local_ip, "127.0.0.1")]
    good = []
    bad_ip = []
    for ip in [a for a in addresses if not a.startswith("127.")]:
        for server_ip in [a for a in server_addresses if not a.startswith("127.")]:
            s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            s.bind((ip,0))
            try:
                s.connect((server_ip, server_port))
                local = s.getsockname()[0]
                remote = s.getpeername()[0]
                if local not in bad_ip and remote not in bad_ip:
                    pair = (local, remote)
                    if pair not in good:
                        good += [pair]
            except:
                print("Ignoring useless ip {}".format(ip))
                for i in range(len(good)):
                    if good[i][0]:
                        bad_ip += [ip]
                        del good[i]
                        break
                pass
            s.close()
    return good


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="TP-Link Wi-Fi Smart Plug")
    parser.add_argument("--host", help="Name of Ada server we want to connect to.", default="ada-core")
    parser.add_argument("--local", help="Override the local ipaddress to use.")
    parser.add_argument("--port", type=int, help="Port we want to connect to on that server", default=12345)
    args = parser.parse_args()

    # Set target IP, port and command to send
    while True:
        try:
            switches = []
            found_server_ip = None
            for local_ip, server_ip in find_local_ips(args.local, args.host, args.port):
                print("Searching network from ip address: {} ...".format(local_ip))
                for addr in TplinkSmartPlug.findHS105Devices(local_ip):
                    found_server_ip = server_ip
                    switches += [(local_ip, addr)]

            if len(switches) == 0:
                print("Could not find your local HS105 switches", flush=True)
                time.sleep(1)
            elif found_server_ip is None:
                print("Cound not find Ada server {}".format(args.host))
            else:
                print("Found switches at: {}".format([x[1] for x in switches]))
                local = switches[0][0]
                print("Using local network on: {}".format(local))
                plug = TplinkServer(local, found_server_ip, args.port)
                plug.start(switches)
                print("Press CTRL+C to terminate...", flush=True)
                while True:
                    time.sleep(1)
        except Exception as e:
            print("Exception: {}".format(e), flush=True)
            time.sleep(1)
