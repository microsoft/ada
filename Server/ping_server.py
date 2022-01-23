# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
import argparse
import socket
import json

def receive_rgb_lists_from_server(server_endpoint, buffer_size):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect(server_endpoint)
        sock.sendall(bytes('GET\n', 'utf-8'))
        json_msg = ''
        while len(json_msg) == 0:
            json_msg = str(sock.recv(buffer_size), "utf-8")
            command = json.loads(json_msg)
            if command["command"] == "sensei":
                colors = command["colors"]
                received_rgb_lists_per_zone = [(x[0], x[1], x[2]) for x in colors]

        return received_rgb_lists_per_zone


def update_server_with_ipcamera_emotion(server_endpoint, emotion, score):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect(server_endpoint)
        sock.sendall(bytes('IPCAMERA {} {}\n'.format(emotion, score), 'utf-8'))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Pulls Sensei data from the ada_server for display on the raspberry pi')
    parser.add_argument("--ip", help="optional IP address of the server (default 'localhost')", default="localhost")
    parser.add_argument("--emotion", help="optional parameter to set the ipcamera emotion in the server", default=None)
    parser.add_argument("--score", help="optional parameter to set the ipcamera score for teh emotion", default=None)
    args = parser.parse_args()

    server_port = 12345
    socket_buffer_size = 32768
    server_endpoint = (args.ip, server_port)
    if args.emotion is not None:
        update_server_with_ipcamera_emotion(server_endpoint, args.emotion, args.score)
    json = receive_rgb_lists_from_server(server_endpoint, socket_buffer_size)

    print(json)