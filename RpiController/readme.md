## RpiController

This is a `cmake` project for building the `RpiController` app which runs on the Raspberry Pi's.

Then check if `RpiController` is already running using `ps -ax` if it is you can kill it so you
can update it as follows:

```
cd /home/pi/git/Ada
git pull
cd RpiController
mkdir build && cd build && cmake ..
make
./bin/RpiController
```

If the Raspberry Pi devices do not have internet access, then you can update them from the
ada-core server PC using the following:

```
git pull
scp -r c:\git\Ada\ pi@adapi1:/home/pi/git/
scp -r c:\git\Ada\ pi@adapi2:/home/pi/git/
scp -r c:\git\Ada\ pi@adapi3:/home/pi/git/
```

The TeensyFirmware will be updated automatically from the Server
but if you want to install new firmware manually then do this:

```
chmod a+x /home/git/Ada/RpiController/run.sh
chmod a+x /home/git/Ada/TeensyFirmware/flash.sh
```

This app automatically finds the Teensy on a Serial port and connects to it then
connects to the Server named in the `~/Ada/settings.json` file.  You will see this file
contains the server name `ada-core` and the local name `adapi3` which is to be sent to the server.

```json
{
  "server": "ada-core",
  "local": "adapi3",
  "server-port": 12345
}
```

When you run the program without `--autostart` option you will see the following output:

```
Looking for server: ada-core...
Using local ip "192.168.1.7"
Using server ip "192.168.1.12"
Found Teensy at: /dev/ttyACM0, Teensyduino_USB_Serial_6194220
Enter one of the following commands:
  q                       exit this program:
  ?                       query status of Teensy:
  s                       get Sensei data over local and server ip addresses
  b s f1 f2               subtle breath animation with seconds and two f-ratios
  t                       run the speed test which measures bytes per second
  r l s                   rainbow animation of given length and seconds
  n i                     neural drop pattern repeated i times
  c R G B i j             set a color, with optional strip and led indexes (defaults to all leds).
  f R G B s               smooth fade to new color over given seconds.
  g s { R G B }*          set a smooth gradient from top to bottom of each strip, animated s seconds.
  z R G B R G B s d       setup the twinkle pattern with base color rgb, star color rgb, speed, and density.
  w s d a                 water droplet over existing color, s=size, d=drops, a=amount of color to add for droplet.
  p { s l R G B }*        set a series of individual pixels to each color, s=strip, l=led.
  rain [on|off] s a       start or stop rain animation overlay with given size and color amount
  0                       run serial speed test.
> |
```

Notice it finds the server, connects to it and waits to receive commands from the user.

## Auto-Start

On the `Raspberry Pi` it is convenient to use this script, because it starts RpiController with
`--autostart` and the RpiController will terminate if it detects the Teensy is not responding or if
a Firmware update is required.  This allows the script to then reflash the Teensy which brings it
back, then it restarts RpiController. So this should keep things running forever.

```
./run.sh
```

## Windows

You can also run the RpiController on Windows with Teensy connected over USB to run a debug LED
strip in your office. To get a Visual Studio solution do this:

```shell
cd /home/pi/git/Ada
git pull
cd RpiController
mkdir build && cd build
cmake -G "Visual Studio 16 2019" -A x64 ..
```

Now you will have a `RpiController.sln` that you can use in the build folder to debug the code.
