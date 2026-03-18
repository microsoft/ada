# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

import asyncio
import json
import queue

import websockets
from azure.messaging.webpubsubservice import WebPubSubServiceClient

from logger import Logger

logger = Logger()
log = logger.get_root_logger()


# Provides bi-directional connection to given Azure Web PubSub service group.
class WebPubSubGroup:
    def __init__(self, webpubsub_constr, hub_name, user_name, group_name, internet_address):
        self.webpubsub_constr = webpubsub_constr
        self.client = None
        self.listeners = []
        self.closed = True
        self.user_name = user_name
        self.hub_name = hub_name
        self.group_name = group_name
        self.ack_id = 1
        self.web_socket = None
        self.send_queue = queue.Queue()
        self.connection_id = None
        self.internet_address = internet_address

    def add_listener(self, handler):
        self.listeners += [handler]

    async def connect(self):
        try:
            log.info("### websocket connecting..")
            self.client = WebPubSubServiceClient.from_connection_string(
                connection_string=self.webpubsub_constr, hub=self.hub_name
            )
            self.closed = False
            token = self.client.get_client_access_token(
                user_id=self.user_name,
                roles=[
                    f"webpubsub.joinLeaveGroup.{self.group_name}",
                    f"webpubsub.sendToGroup.{self.group_name}",
                ],
            )
            uri = token["url"]
            self.web_socket = await websockets.connect(
                uri, subprotocols=["json.webpubsub.azure.v1"]
            )
            await self._join_group()

            log.info("### websocket connected.")
        except Exception as e:
            log.error(f"### web socket connect failed:{e}")
            await self.close(False)
            await asyncio.sleep(10)
        return

    def send(self, msg):
        groupMessage = {
            "type": "sendToGroup",
            "group": self.group_name,
            "dataType": "json",
            "data": msg,
            "ackId": self.ack_id,
            "ip": self.internet_address,
        }
        self.ack_id += 1
        self.send_queue.put(groupMessage)

    async def consume(self):
        while not self.closed:
            if not self.send_queue.empty():
                message = self.send_queue.get()
                if self.web_socket:
                    if message:
                        data = None
                        try:
                            data = json.dumps(message)
                        except Exception as e:
                            log.error("### cannot serialize data: ", message)
                            continue

                        try:
                            await self.web_socket.send(data)
                        except Exception as e:
                            # websocket has been closed?
                            log.error("### websocket send error: ", e)
                            self.close(False)
                            # socket should be recreated by the listen thread.
                            await asyncio.sleep(10)
                else:
                    # drop it on the floor and wait for reconnect in listen thread.
                    await asyncio.sleep(10)
            else:
                await asyncio.sleep(0.1)

        await self.close()

    async def listen(self):
        log.info("Listening for messages from WebSocket...")
        while not self.closed:
            if self.web_socket is None:
                await self.connect()  # auto-reconnect!

            if not self.web_socket:
                # try again in 60 seconds, might be temporary Azure or network outage?
                await asyncio.sleep(60)
                continue

            try:
                async for message in self.web_socket:
                    try:
                        self._handle_message(message)
                    except Exception as e:
                        log.info("### handle message error: ", e)

            except Exception as e:
                # websocket has been closed?
                log.info("### websocket receive error: ", e)
                await self.close(False)
                await asyncio.sleep(10)
        log.info("Stopped listening to WebSocket.")

    def _handle_message(self, data):
        log.info("Message received: " + data)
        message = json.loads(data)
        if "fromUserId" in message:
            user = message["fromUserId"]
            if user != self.user_name:
                for h in self.listeners:
                    h(user, message)

    async def _join_group(self):
        log.info(f"### websocket joining group {self.group_name}...")
        msg = {"type": "joinGroup", "ackId": self.ack_id, "group": self.group_name}
        if not self.web_socket:
            return {}
        data = json.dumps(msg)
        await self.web_socket.send(data)
        response = await self.web_socket.recv()
        try:
            response = json.loads(response)
            self.ack_id += 1
            # now we should have the connection id and an idea of success
            if "event" in response and response["event"] == "connected":
                self.connection_id = response["connectionId"]
                log.info(f"### websocket joined group id = {self.connection_id}")
            else:
                # TODO: how to recover from this?
                log.warning(f"Unexpected joinGroup response: {response}")
        except:
            # TODO: how to recover from this?
            log.warning(f"error parsing joinGroup response as json")
            return {}

    async def close(self, closed=True):
        self.closed = closed
        socket = self.web_socket
        self.web_socket = None
        if socket:
            try:
                await socket.close()
            except Exception as e:
                log.error(f"### error closing web_socket {e}")
        if self.client:
            try:
                self.client.close()
            except Exception as e:
                log.error(f"### error closing client {e}")
            self.client = None
