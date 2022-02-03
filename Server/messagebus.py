# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

# Provides bi-directional connection to given signalr service
import requests
import json
import time
from collections import namedtuple
import sys
import os
from threading import Thread
import priority_queue


script_dir = os.path.dirname(os.path.abspath(__file__))
sys.path += [os.path.join(script_dir, "signalrcore")]
import signalrcore
from signalrcore.hub_connection_builder import HubConnectionBuilder


class MessageBus:
    def __init__(self, uri):
        self.connection = None
        self.uri = uri
        self.listeners = []
        self.closed = True
        self.delay = 0
        self.thread = None
        self.send_queue = priority_queue.PriorityQueue()
        self.stopped = False

    def add_listener(self, handler):
        self.listeners += [handler]

    def get_access_token(self):
        return self.accessToken

    def connect(self):
        self.session = requests.Session()
        response = requests.get(f"{self.uri}/negotiate")
        msg = response.content.decode('utf-8')
        data = json.loads(msg)
        server_url = data['url']
        self.accessToken = data['accessToken']
        self.stop()
        # .configure_logging(logging.DEBUG)\
        self.connection = HubConnectionBuilder()\
            .with_url(
                server_url,
                options={
                    "access_token_factory": self.get_access_token,
                })\
            .with_automatic_reconnect({
                "type": "raw",
                "keep_alive_interval": 10,
                "reconnect_interval": 5,
                "max_attempts": 5
            }).build()
        self.connection.on_open(self.opened)
        self.connection.on_close(self.closed)
        self.connection.on_error(self.error)
        self.connection.on("log", self.received)
        self.connection.start()
        self.closed = False  # assume it will succeed so we don't try again until we know for sure.
        self.stopped = False

    def opened(self):
        print("### signalr connection opened and handshake received ready to send messages")
        self.closed = False

    def closed(self):
        print("### signalr connection closed")
        self.closed = True
        self.stop()

    def stop(self):
        if self.connection:
            try:
                self.connection.stop()
            except:
                pass
        self.connection = None
        self.stopped = True

    def error(self, e):
        print("### signalr error: ", e)
        self.closed = True
        self.stop()  # we'll do a reconnect later.

    def check_connection(self):
        if not self.closed and self.delay != 0 and self.delay > time.time():
            self.reconnect()

    def received(self, msg):
        if type(msg) == list:
            msg = msg[0]
        try:
            data = json.loads(msg)
            data = namedtuple("Message", data.keys())(*data.values())
            if hasattr(data, 'user') and hasattr(data, 'message'):
                for h in self.listeners:
                    h(data.user, data.message)
        except Exception as e:
            print("### signalr parse error:", e)

    def reconnect(self):
        try:
            if self.delay != 0 and self.delay > time.time():
                self.connect()
                return True
            else:
                # skip reconnect attempt until delay is reached.
                return False
        except:
            print("### signalr connection failed, trying again in 10 seconds")
            self.delay = time.time() + 10
            return False

    def send(self, user, msg):
        if type(msg) == str:
            body = f"{{\"user\": \"{user}\", \"message\": \"{msg}\"}}"
        else:
            # embed the json for the message into the json block
            text = json.dumps(msg)
            body = f"{{\"user\": \"{user}\", \"message\": {text}}}"

        self.send_queue.enqueue(0, body)
        if self.thread is None:
            # send is not using signalr, so it has no connection to
            self.thread = Thread(target=self._send_thread)
            self.thread.daemon = True
            self.thread.start()

    def _send_thread(self):
        while not self.stopped:
            item = self.send_queue.dequeue()
            if item is None:
                time.sleep(0.1)
            else:
                try:
                    _, body = item
                    requests.post(f"{self.uri}/post", body)
                except Exception as e:
                    print("### signalr post failed :", e)


if __name__ == "__main__":
    bus = MessageBus("http://localhost:7071/api/ada")
    bus.connect()
    while True:
        msg = input("Enter a message: ")
        bus.send('server', msg)
