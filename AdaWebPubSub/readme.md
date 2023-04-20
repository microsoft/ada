# Ada Web Pub Sub

The [AdaKiosk](../AdaKiosk/readme.md) communicates with the [AdaServer](../Server/readme.md) and vice versa
using an [Azure Web Pub Sub service](https://azure.microsoft.com/en-us/services/web-pubsub/).

To use this service you must know the `ADA_WEBPUBSUB_CONNECTION_STRING` and then you can use the
[messagebus.py](../AdaServer/messagebus.py) this:

```python
import os
from messagebus import WebPubSubGroup

constr = os.getenv('ADA_WEBPUBSUB_CONNECTION_STRING')
msgbus = WebPubSubGroup(constr, 'AdaKiosk', 'test', 'demogroup')
await msgbus.connect()
```

Then you can start listening for messages:

```python
msgbus.add_listener(on_message)

# run the async listener
await asyncio.gather(
    msgbus.listen(),
    msgbus.consume())
```
Where the listener receives 2 arguments, the user and the message as follows:
```python
def on_message(user, message):
    print(f'{user}: {message}')
```

When you receive a message from Ada, it might be telling you about power up / down events,
or it might be telling you someone else sent an animation command.

```python
def onmessage(self, user, msg):
    if user == 'test':
        # ignore our own messages.
        return
    ...
```

To send a command to Ada use the send function like this:

```python
msgbus.send('/state/on')  # power up Ada!
```

# Commands

Here is a list of commands you can send:

| command      | Function |
|--------------|----------|
| `/ping`      | Check status of the server     |
| `/state/on`    | Power up the Ada structure     |
| `/state/off`   | Power down the Ada structure |
| `/state/run`   | Put Ada into auto mode (running it's own animations) |
| `/state/reboot` | Start a 2 minute cool down and reboot cycle |
| `/state/bridge` | Ping the wifi bridge to make sure it is running |
| `/state/bridge` | Ping the wifi bridge to make sure it is running |
| `/animation/moving gradient cycle` | start the animation defined in [Server config.json](../Server/config.json) file |
| `{"command": "StartRain", "size": 12, "amount": 50.0}` | Start a rain animation on top of whatever else is happening |
| `{"command": "StopRain"}` | Stop the rain animation overlay |
| `{"command": "Rainbow", "seconds": 0, "hold": seconds, "length": 157}` | Start an infinite rainbow animation |
| `{"command": "sensei", "target": "DMX", "seconds": seconds, "colors": rgb_colors, "hold": hold}` | Send 6 colors to each DMX light where each color is a string containing `r,g,b` |
| `{"command": "CrossFade", "colors": rgb_colors, "seconds": seconds, "hold": hold}` | Smooth fade to new colors |
| `{"command": "Gradient", "colors": colors, "seconds": seconds }` | Set vertical gradient on each strip to the given colors |
| `{"command": "MovingGradient", "colors": [ [ 255, 255, 0 ],[ 0, 128, 255 ],[ 0, 0, 255 ]], "speed": 1, "direction": -1,"size": 50}` | Animate each strip over 1 second until they are showing this gradient, the direction specifies top to bottom (1) or bottom to top (-1) |

## Animations

There are 11 built in animations defined in the [Server config.json](../Server/config.json) file.  We can easily add new animation groups there and publish a new config.json to the server machine, this
can simplify your commands since you can simply reference them by name.

The [Server config.json](../Server/config.json) currently contains the following animations:

- /animation/neurons
- /animation/rainbow
- /animation/fire
- /animation/red yellow gradient
- /animation/blue red gradient
- /animation/yellow green gradient
- /animation/blue green gradient
- /animation/diagonal gradient
- /animation/moving gradient 1
- /animation/moving gradient 2
- /animation/gradient cycle
- /animation/moving gradient cycle
- /animation/color cycle

The 'cycle' animations are compound animations that contain multiple commands, for example `moving gradient cycle`
contains 11 separate animations which are cycled through with each one separated by the 10 seconds delay defined on each one.

# Low level commands

| command      | Function |
|--------------|----------|
| `{"command": "SetPixels", "target": target, "pixels": pixels}` | Send specific pixels to a specific raspberry pi |
| `{"command": "ColumnFade", "target": target, "seconds": 0, "columns": columns}` | Fade to new colors on specific columns of specific target raspberry pi devices where columns is an array of `{"index": i, "color": color}` and color is a string containing `r,g,b` |


# Messages

Here some of the messages you might see from the system:


| message      | user   | Function |
|--------------|--------|----------|
| `{'type': 'message', ... 'data': '/animation/moving gradient cycle'}` | kiosk | Someone selected this animation from the Kiosk |
| `{'type': 'message', ... 'data': [{'command': 'MovingGradient', 'colors': [[128, 0, 255], [0, 0, 255]], 'speed': 2, 'direction': 1, 'size': 50, 'seconds': 10 }]}` | server | The server is performing the next step in the moving gradient cycle animation |


Notice that the notifications relayed from the server often include the `command` that was sent to the server, or
a command that the server is performing on it's own in response to it's programming or a higher level animation that
was specified.


