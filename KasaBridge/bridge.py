# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
import argparse
import socket
import time
import datetime
import sys
import _thread
from tplink_smartplug import TplinkSmartPlug
from internet import wait_for_internet, get_local_ip


class TplinkServer:
    def __init__(self, local_ip, server_ip, server_port):
        self.local_ip = local_ip
        self.server_endpoint = (server_ip, server_port)

    def log(self, msg):
        timestamp = datetime.datetime.now().strftime("%x %X")
        print("{}: {}".format(timestamp, msg), flush=True)

    def start(self, plugs):
        self.log("starting server on : {}".format(self.server_endpoint))
        self.closed = False
        self.plugs = plugs
        _thread.start_new_thread(self.serve_forever, ())

    def stop(self):
        self.closed = True

    def serve_forever(self):
        while not self.closed:
            try:
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.bind((self.server_endpoint[0], 0))
                s.settimeout(5)
                s.connect(self.server_endpoint)
                s.sendall(bytes("HS105Switches", "utf-8"))
                while True:
                    try:
                        request = s.recv(16000)
                        if request is not None:
                            msg = request.decode("utf-8")
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
                            else:
                                response = "unknown request: " + request
                            s.sendall(bytes(response, "utf-8"))
                    except socket.timeout:
                        # totally normal, since out socket has a timeout value of 1 minute.
                        time.sleep(1)

            except Exception as e:
                self.log("## bridge exception: {}".format(e))
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
                    self.log("Turning on {}...".format(switch_ip))
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
                    self.log("Turning off {}...".format(switch_ip))
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
            status += ["{}:{}".format(switch_ip, plug.is_on == 1)]
        return ",".join(status)


def find_local_ips(local_ip, server_name, server_port):
    server_addresses = socket.gethostbyname_ex(server_name)[2]
    hostname = socket.gethostname()
    addresses = socket.gethostbyname_ex(hostname + ".local")[2]
    if local_ip:
        addresses = [local_ip]
        if server_name.startswith("127.") or server_name == "localhost":
            return [(local_ip, "127.0.0.1")]
    good = []
    bad_ip = []
    if local_ip in server_addresses:
        return [(local_ip, local_ip)]
    for ip in [a for a in addresses if not a.startswith("127.")]:
        for server_ip in [a for a in server_addresses if not a.startswith("127.")]:
            s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            s.bind((ip, 0))
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


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="TP-Link Wi-Fi Smart Plug")
    parser.add_argument(
        "--host", help="Name of Ada server we want to connect to.", default="ada-core"
    )
    parser.add_argument("--local", help="Override the local ipaddress to use.")
    parser.add_argument(
        "--port",
        type=int,
        help="Port we want to connect to on that server",
        default=12345,
    )
    parser.add_argument("--server", help="Server ip address to use", default=None)
    parser.add_argument(
        "--test", help="Protend we did find some switches", action="store_true"
    )
    args = parser.parse_args()

    wait_for_internet()
    local_ip = args.local
    if local_ip is None:
        local_ip = get_local_ip()

    retry_time = 10  # seconds

    # Set target IP, port and command to send
    while True:
        try:
            switches = []
            found_server_ip = args.server
            if args.test and (not found_server_ip or not local_ip):
                print("### --test requires --server and --local arg to be set")
                sys.exit(1)
            if args.test:
                switches += [(local_ip, "192.168.1.199")]
            else:
                for local_ip, server_ip in find_local_ips(
                    local_ip, args.host, args.port
                ):
                    print("Searching network from ip address: {} ...".format(local_ip))
                    for addr in TplinkSmartPlug.findHS105Devices(local_ip):
                        if not found_server_ip:
                            found_server_ip = server_ip
                        if (local_ip, addr) not in switches:
                            switches += [(local_ip, addr)]

            if len(switches) == 0:
                print("Could not find your local HS105 switches", flush=True)
                time.sleep(retry_time)
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
            time.sleep(retry_time)
