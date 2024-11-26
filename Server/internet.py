import socket
import time


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


def get_local_ip():
    wait_for_internet()
    local_ip = socket.gethostbyname(socket.gethostname())
    print(f"Using local ip address {local_ip}")
    return local_ip


def main():
    get_local_ip()


if __name__ == "__main__":
    main()
