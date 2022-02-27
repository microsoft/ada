## Server

This project contains the python code that runs on the `ada-core` server PC.

The server PC has an [Anaconda](https://www.anaconda.com/distribution/) python environment named
"Ada".  Run the "pip install -r requirements.txt" using requirements.txt
from the parent folder.

This server uses an Azure Web Pub Sub service to do realtime communication with the AdaKiosk app.  See the
[readme](../Azure/readme.md) to set that up.  This server
then uses the ADA_WEBPUBSUB_CONNECTION_STRING environment variable to connect to the Web Pub Sub service.

To launch the server:

- Bring up an Anaconda command prompt
- activate ada
- cd \git\Ada\Server
- git pull
- python ada_server.py --ip 192.168.1.11

(where 192.168.1.11 is the ip address of ada-core machine on the private network, so check the
address is the same - sometimes it changes).

Add the `--loop` option to run a "replay" mode using the *.csv files in the `HistoricalData` folder.

Add the `--web` option to start a web server on the ada-core machine that
provide a convenient web page for overriding the Ada colors.

It provides a TCP server that the RaspberryPi's and the IPCameraGUI, the DMX Controller and the
KasaBridge connect to to send/receive commands.

## Config

The `config.json` file contains the mapping of raspberry pi's to zones, and the color mappings and
lots of other timing settings.  The Server is responsible for mapping the zones to led strips on
each pi, and sends `ColumnFade` commands to set those colors on each Pi.  The Server also decides
when to turn on the entire center core or just the led's connected to optical fiber bundles. All
this is managed by the `LightingDesigner` class in the ada_server.py script.

This is the full list of settings you can play with:

- `connection_string` - the Azure Sensei database connection (for data from kitchens)
- `camera_zones` - the camera zones, can contain more than one camera in each zone, these must match the data in Azure.
- `colors_for_emotions` - mapping of emotion to color for the LED's.  These colors have to be muted because the LEDs show
full brightness at about a value of 128.  Changing from 128 to 255 shows very little difference.
- `colors_for_dmx_emotions` - mapping of colors to DMX lighting colors, these can be brighter.
- `server_port` - the port used by the ada_server.py script, used by raspberry pi's to connect.
- `pubsub_group`: The name of the pubsub group to use.
You might want to change this while running unit tests
so you don't confuse any production Ada servers.
- `pubsub_hub`: The name of the pubsub Hub.
- `on_time` - time the lights turn on in the morning.
- `off_time` - time the lights turn off at night.
- `turn_off_timeout` - cool down period for the power supplies before power off
- `tplink_bridge_address` - address of the raspberry pi zero "KasaBridge" device.
- `tplink_bridge_port` - port that the KasaBridge is listening on for commands from the server.
- `zone_maps` - the mapping from raspberry pi name to the zone map used for that pi.
- `history_dir` - location of historical sensei data used in `--loop` mode.
- `playback_delay` - delay between each row of historical data.
- `rainbow_timeout` - when 3 faces are detected play a rainbow animation for this many seconds.
- `cool_animation_timeout` - when playing cool animations, this is how long (in seconds).
- `cool_animation_time` - when to start playing cool animations instead of showing Sensei data.
- `movement_rain_timeout` - when movement is detected a `rain` animation plays for this long (in seconds)
- `playback_weights` - this is a fudge to get interesting colors our of the historical data.
- `cool_animations` - the list of cool animations that we randomly choose from.

## Localhost debugging

You can run the `ada_server.py` with ip "localhost" on your dev box to
debug it and the RpiController on the same machine with the RpiController
talking to a Teensy over USB.  This makes for a convenient debugging environment.

It is recommended you change the configuration for
`pubsub_hub` and `pubsub_group` while testing so you don't
confuse the real Ada server.


## UnitTest mode

The simplest unit test is to run `python ada_server.py` in a terminal window and
run the [AdaKiosk](../AdaKiosk/readme.md) at the same time.  These will connect to the
same Azure Web Pub Sub service and and when you click the AdaKiosk "Simulation" tab
you will see a rendering of all the colors being set by the `ada_server`.

You can also run the server with an RpiController with [TeensyUnitTest](../TeensyUnitTest/readme.md).
This provides a way to test everything end to end.
