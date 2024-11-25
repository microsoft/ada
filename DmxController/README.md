
## DMX controller

The `dmx_control.py` script gets commands from the Server and drives the big par lights using a DMX connection over USB.

To launch the DMX controller run this from an `Ada` conda environment:

```
python dmx_control.py --port ...
```

On Windows it will be a COM port.

On Linux the port name can be found under:
```
 ls /dev/serial/by-id/
```
It should show something like `usb-DMXking.com_DMX_USB_PRO_6A3ZO5R6-if00-port0`.  So you would run
```
python dmx_control.py --port /dev/serial/by-id/usb-DMXking.com_DMX_USB_PRO_6A3ZO5R6-if00-port0
```

## Setup

Only pyserial. We've tested this on Windows an Raspberry Pi, without needing to install any drivers on either platform (I think it's just an FTDI chip).
```
pip install pyserial
```

On Linux you need to also do the following:
```
sudo usermod -a -G dialup $USER
sudo gpasswd --add $USER dialout
```

## dmx.py

The `dmx.py` script provides the API we are using to talk tot he lights.
This is a bit of example code:

```python
from dmx import Dmx, DmxDevice
# In Linux, use the symlinks in /dev/serial/by-id/ instead of /dev/serialX
DMX_SERIAL_PORT = '/dev/serial/by-id/usb-ENTTEC_DMX_USB_PRO_Mk2_EN252261-if00-port0'
dmx = Dmx(DMX_SERIAL_PORT)
```

The API might seem a bit weird, but it was useful to do it this way for our particular setup. In particular, it assumes that the address space is contiguous, and you don't specify specific channel numbers.

This is how you add devices:

```python
DMX_UNIVERSE = 1
dmx.add_device(DMX_UNIVERSE, DmxDevice("color_lights_front_side_left", "color_lights", 5, 2))
dmx.add_device(DMX_UNIVERSE, DmxDevice("infra_lights_front_side_left", "infra_lights", 5, 2))
```

add_device takes a DMX_UNIVERSE parameter, which be 1 or 2, mapping to the two outputs of the DXM MK2 controller. Each gets an address space with a range of 1 to 512 channels that can exist on a single universe.

The second parameter is a DmxDevice, which has this constructor:

```python
class DmxDevice:
    def __init__(self, device_name: str, device_type: str, output_channels: int, bytes_per_channel: int):
```

The device_name is a unique identifier for each decoder. The device_type a class of thing you might want to address as a group. You can then address things by either.

The decoders we used each had five output channels, and could be configured to work in 16-bit resolution, which is why output_channels is 5 and bytes_per_channel is 2.

In our case, that means that a single decoder takes up 10 addresses, which map to:
```
# 5-Channel, 16-bit output mapping to RGBWC LED strips
#
# 1 RED MSB
# 2 RED LSB
# 3 GREEN MSB
# 4 GREEN LSB
# 5 BLUE MSB
# 6 BLUE LSB
# 7 WARM MSB
# 8 WARM LSB
# 9 COOL MSB
# 10 COOL LSB
```
So, the first device will use up address 1 to 10, and the second 11 to 20 etc. Of course you need to physically configure the decoders to make sure they're using these channels.

You can add devices of different output_channels and bytes_per_channel. For example, if I added one with and 2-channel 8-bit control, it would then take up addresses 21 and 22.

Another example, this is a set of four devices - the first two use 4 x 8-bit channels outputs to control mains dimming socket. The second two only have 1 output:

```python
DMX_UNIVERSE = 2

dmx.add_device(DMX_UNIVERSE, DmxDevice("rearing_heater", "appliances", 4, 1))
dmx.add_device(DMX_UNIVERSE, DmxDevice("rearing_fogger", "appliances", 4, 1))
dmx.add_device(DMX_UNIVERSE, DmxDevice("uv_internal_trap", "appliances", 1, 1))
dmx.add_device(DMX_UNIVERSE, DmxDevice("uv_external_trap", "appliances", 1, 1))
```

So the first uses addresses 1-4, the second 5-8, the third 9 and the fourth 10.

If you have two devices with non-contiguous address spaces, you need to declare a dummy DmxDevice to take up that unused range, and add it between the two real devices.

To actually control things, you have two options:

```python
set_output_by_type(self, dmx_universe: int, device_type: str, output_values)
```
or
```
set_output_by_name(self, dmx_universe: int, device_name: str, output_values):
```

Where output_values is an array of values. If you are setting only one value (a single-output, 8-bit resolution decoder), you need to cast it as an array, e.g.

```python
value = 255 if self.rearing_heater_state else 0
dmx.set_output_by_name(DMX_UNIVERSE, "rearing_heater", [value])
dmx.send_update(DMX_UNIVERSE)
```

An example of setting all the channels in a 5-channel, 16-bit resolution decoder:

```python
# RGBWW values come in as floats in range 0.0-1.0, map to an array of 16-bit uints (0-65535)
light_color_16_bit_array = [int(float(x) * 65535.0) for x in (red, green, blue, warm_white, cold_white)]
dmx.set_output_by_type(DMX_UNIVERSE, "color_lights", light_color_16_bit_array)
dmx.send_update(DMX_UNIVERSE)
```

## Final Step

The final call that actually updates the values is:

```python
dmx.send_update(DMX_UNIVERSE)
```
Once the USB controller receives the data it will broadcast it over DMX. Once all the decoders receive the final END_CODE value, they synchronously update their output state. The DMX controller will automatically and periodically (like ever second) re-transmit the last specified values to the DMX network. So if a decoder misses a packet or joins late after the initial transmission, it will receive it on the next auto re-transmit.
