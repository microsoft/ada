import socket
import time
from urllib.parse import urlparse


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


def get_ip_address(url: str):

    try:
        parsed_url = urlparse(url)
        hostname = parsed_url.hostname
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect((hostname, 80))
        ip = s.getsockname()[0]
        s.close()
        print(f"Found internet via local ip {ip}")
        return ip
    except Exception as e:
        print(str(e))
        time.sleep(10)
        return str(e)
