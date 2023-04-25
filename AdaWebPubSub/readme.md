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
| `/animation/moving gradient cycle` | start the animation defined in [Server config.json](../Server/config.json) file, see animation names below |
| `/emotion/name` | set the color associated with the given emotion where the built in names are defined in the [Server config.json](../Server/config.json) are ['anger', 'contempt', 'disgust', 'fear', 'happiness', 'neutral', 'sadness', 'surprise' ]
| `/rain/on` | Start a rain animation on top of whatever else is happening |
| `/rain/off` | Stop the rain animation overlay |
| `/color/r,g,b` | Set given r,g,b color on all of Ada (e.g. `/color/255,255,0` will set Yellow) |
| `/dmx/r,g,b` | Set DMX lights to the given r,g,b values, pass 6 separate r,g,b tuples separated by `/` to send different colors to each DMX light |
| `/zone/index/r,g,b` | Set the specified zone to the given r,g,b colors, there are 6 zones (0-5) defined in Server for each raspberry pi named zone_map_1.json, zone_map_2.json and zone_map_3.json |
| `/strip/index/r,g,b` | Set a specific strip (index 0-48) on Ada to the given color, some strips are in the center cone |
| `/gradient/target/r,g,b/r,g,b/...` | Send the given gradient colors to the named target where the names are `adapi`, `adapi2`, `adapi3` and `DMX`. Pass target `*` to send it to all devices. |
| `/pixels/{t/strip/ledranges/color}` | Can be used to set specific pixels on a given strip on a target raspberry pi where `t` is an index `0,1,2`, and strip is an index `0-15` on the targeted Teensy, and `ledranges` is a comma separated set of led indexes or index ranges specified with hyphen and color is a single `r,g,b` value.  For example `/pixels/0/1/1,2-10,30,40-100/255,0,0` sets pixel 1, 2 through 10, 30, and 40 through 100 to red on led strip 1 of the first raspberry pi.

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

# Messages

Here some of the messages you might see from the system:

| message      | user   | Function |
|--------------|--------|----------|
| `{'type': 'message', ... 'data': '/animation/moving gradient cycle'}` | kiosk | Someone selected this animation from the Kiosk |
| `{'type': 'message', ... 'data': [{'command': 'MovingGradient', 'colors': [[128, 0, 255], [0, 0, 255]], 'speed': 2, 'direction': 1, 'size': 50, 'seconds': 10 }]}` | server | The server is performing the next step in the moving gradient cycle animation |


Notice that the notifications relayed from the server often include the `command` that the server is performing in response a higher level animation that was specified.
