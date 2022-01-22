import socket
import sys

def findDMXDevices():
    #listen for Art-Net broadcast
    artnet_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    artnet_sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

    artnet_sock.bind(('', 6454))

    while True:
        data, addr = artnet_sock.recvfrom(1024) # buffer size is 1024 bytes
        if len(data) >= 7:
            header = data[:7].decode('utf8')
            if header == "Art-Net":
                yield addr

if __name__ == "__main__":
    for addr in findDMXDevices():
        print(addr)
