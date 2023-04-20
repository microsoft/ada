import os
import asyncio
import _thread
import sys
from messagebus import WebPubSubGroup

CONNECTION_STRING_NAME = 'ADA_WEBPUBSUB_CONNECTION_STRING'


def on_message(user, message):
    print(f'{user}: {message}')


def prompt_for_command(input_queue):
    while True:
        cmd = input("Enter command to send or 'x' to exit: ")
        input_queue.put_nowait(cmd.strip())
        if cmd == 'x':
            return


async def async_command_line(msgbus):
    # console input has to be in a separate thread otherwise it somehow
    # blocks all asyncio, including the msgbus.listen task.
    input_queue = asyncio.Queue()
    _thread.start_new_thread(prompt_for_command, (input_queue,))
    while True:
        cmd = await input_queue.get()
        if cmd == 'x':
            msgbus.close()
            return
        else:
            msgbus.send(cmd)


async def _main():
    constr = os.getenv(CONNECTION_STRING_NAME)
    if not constr:
        raise Exception(f'{CONNECTION_STRING_NAME} environment variable not set')

    msgbus = WebPubSubGroup(constr, 'AdaKiosk', 'test', 'demogroup')
    await msgbus.connect()
    msgbus.add_listener(on_message)
    await asyncio.gather(async_command_line(msgbus), msgbus.listen(), msgbus.consume())


if __name__ == '__main__':
    asyncio.get_event_loop().run_until_complete(_main())
