import socket
import time

from netifaces import AF_INET, ifaddresses, interfaces


def wait_for_internet():
    while True:
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            s.connect(("8.8.8.8", 80))
            ip = s.getsockname()[0]
            s.close()
            print(f"Found internet on local ip {ip}")
            return ip
        except Exception as e:
            print(str(e))
            time.sleep(10)
            return str(e)


def get_ip_addresses():
    result = {}
    for ifaceName in interfaces():
        addresses = [i["addr"] for i in ifaddresses(ifaceName).setdefault(AF_INET, [{"addr": None}])]
        non_none = [x for x in addresses if x is not None]
        if non_none:
            result[ifaceName] = non_none
    return result
