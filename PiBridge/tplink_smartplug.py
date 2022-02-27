###################################################################################################
#
#  Project:  Embedded Learning Library (ELL)
#  File:     tplink_smartplug.py
#  Authors:  Chris Lovett, Lubomir Stroetmann
#
#  Requires: Python 3.x
#
###################################################################################################
import argparse
import socket
import time
import struct
import json

_commands = {'info'     : '{"system":{"get_sysinfo":{}}}',
             'on'       : '{"system":{"set_relay_state":{"state":1}}}',
             'off'      : '{"system":{"set_relay_state":{"state":0}}}',
             'cloudinfo': '{"cnCloud":{"get_info":{}}}',
             'wlanscan' : '{"netif":{"get_scaninfo":{"refresh":0}}}',
             'time'     : '{"time":{"get_time":{}}}',
             'schedule' : '{"schedule":{"get_rules":{}}}',
             'countdown': '{"count_down":{"get_rules":{}}}',
             'antitheft': '{"anti_theft":{"get_rules":{}}}',
             'reboot'   : '{"system":{"reboot":{"delay":1}}}',
             'reset'    : '{"system":{"reset":{"delay":1}}}',
             'energy'   : '{"emeter":{"get_realtime":{}}}'
}


class TplinkSmartPlug(object):
    def __init__(self, local_ip, switch_ip):
        self.local_ip = local_ip
        self.switch_ip = switch_ip
        self.port = 9999
        self.is_on = False

    # TP-Link Smart Home Protocol is an XOR Autokey Cipher with starting key = 171
    @staticmethod
    def encrypt(message, include_length=True):
        key = 171
        ascii_bytes = message.encode()
        if include_length:
            result = bytearray(struct.pack('>I', len(ascii_bytes)))
        else:
            result = bytearray()
        for a in ascii_bytes:
            cipherbyte = key ^ a
            key = cipherbyte
            result.append(cipherbyte)
        return result

    @staticmethod
    def decrypt(ciphertext):
        key = 171
        result = []
        for i in ciphertext:
            plainbyte = key ^ i
            key = i
            result.append(plainbyte)
        return bytes(result).decode()

    @staticmethod
    def getLocalIpAddress():
        import socket
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        addr = s.getsockname()[0]
        s.close()
        return addr

    @staticmethod
    def findHS105Devices(local_ip, timeout=5):
        # this should also work, but it doesn't seem to when raspberry pi is an access point.
        broadcast_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        broadcast_sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        broadcast_sock.settimeout(1)

        broadcast_sock.bind((local_ip, 9999))
        hello = TplinkSmartPlug.encrypt(_commands["info"], False)

        broadcast_sock.sendto(hello, ('<broadcast>', 9999))
        start = time.time()
        result = []
        while time.time() < start + timeout:
            try:
                data, addr = broadcast_sock.recvfrom(1024)
                if len(data) >= 0 and not addr[0].startswith("169."):
                    msg = TplinkSmartPlug.decrypt(data)
                    d = json.loads(msg)
                    if "system" in d:
                        sys = d["system"]
                        if "get_sysinfo" in sys:
                            info = sys["get_sysinfo"]
                            if info is not None and "model" in info:
                                model = info["model"]
                                if model is not None:
                                    if "HS105" in model or "HS103" in model or "EP10" in model:
                                        ip = addr[0]
                                        if ip not in result:
                                            result += [ip]
            except socket.timeout:
                # send another one in case a switch missed the previous UDP broadcast.
                broadcast_sock.sendto(hello, ('<broadcast>', 9999))

        broadcast_sock.close()
        return result

    def get_info(self):
        self.info = json.loads(self.send_command("info"))
        if "system" in self.info:
            system = self.info["system"]
            if "get_sysinfo" in system:
                sysinfo = system["get_sysinfo"]
                if "relay_state" in sysinfo:
                    self.is_on = sysinfo["relay_state"]
        return self.info

    def turn_on(self):
        retries = 5
        while retries > 0:
            result = self.send_command("on")
            time.sleep(0.5)
            self.get_info()
            if self.is_on:
                return
            retries -= 5
        return result

    def turn_off(self):
        retries = 5
        while retries > 0:
            result = self.send_command("off")
            time.sleep(0.5)
            self.get_info()
            if not self.is_on:
                return
            retries -= 5
        return result

    def send_command(self, command):
        return self.send_receive(_commands[command])

    def send_receive(self, message):
        # Send command and receive reply
        while True:
            try:
                sock_tcp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock_tcp.bind((self.local_ip, 0))
                sock_tcp.connect((self.switch_ip, self.port))
                sock_tcp.sendall(TplinkSmartPlug.encrypt(message))
                data = sock_tcp.recv(2048)
                sock_tcp.close()
                if len(data) > 4:
                    return TplinkSmartPlug.decrypt(data[4:])
                else:
                    time.sleep(1)
            except socket.error:
                print("Could not connect to host {}:{}".format(self.switch_ip, self.port))
                time.sleep(1)


if __name__ == '__main__':
    # Parse commandline arguments
    parser = argparse.ArgumentParser(description="TP-Link Wi-Fi Smart Plug")
    parser.add_argument("-l", "--local", metavar="<ipaddress>", help="Override for local IP address to use")
    parser.add_argument("-t", "--target", metavar="<hostname>", help="Target hostname or IP address", default="localhost")
    parser.add_argument("-c", "--command", metavar="<command>", help="Preset command to send. Choices are: "+", ".join(_commands), choices=_commands)
    parser.add_argument("-f", "--find", help="Find local HS105 devices", action="store_true")
    args = parser.parse_args()

    # Set target IP, port and command to send
    ip = args.target
    local_ip = args.local
    plug = TplinkSmartPlug(ip, None)

    if args.find:
        if not local_ip:
            local_ip = TplinkSmartPlug.getLocalIpAddress()
        print("Looking for HS105 devices on network {}".format(local_ip))
        found = False
        for addr in TplinkSmartPlug.findHS105Devices(local_ip, 5):
            found = True
            print("Found device at {}".format(addr))
        if not found:
            print("Nothing found.")
    else:
        response = plug.send_command(args.command)
        print(response)
