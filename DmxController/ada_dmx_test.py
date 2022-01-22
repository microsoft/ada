import time
from dmx import Dmx, DmxDevice

# In RPi, use the symlinks in /dev/serial/by-id/ instead of /dev/serialX
DMX_SERIAL_PORT = 'COM3'

dmx = Dmx(DMX_SERIAL_PORT)

DMX_UNIVERSE = 1

# This device will be assigned start DMX address 1, and will use up to address 6
dmx.add_device(DMX_UNIVERSE, DmxDevice("light_1", "power_lights", 6, 1))

# This device will be assigned start DMX address 7 (6 from the previous one, + 1)
# and will use up to address 12
dmx.add_device(DMX_UNIVERSE, DmxDevice("light_2", "power_lights", 6, 1))

# This device will be assigned start DMX address 13 (6 from the previous one, + 1)
# and will use up to address 12
dmx.add_device(DMX_UNIVERSE, DmxDevice("light_3", "power_lights", 6, 1))

# This device will be assigned start DMX address 19 (6 from the previous one, + 1)
# and will use up to address 18
dmx.add_device(DMX_UNIVERSE, DmxDevice("light_4", "power_lights", 6, 1))

# This device will be assigned start DMX address 29 (6 from the previous one, + 1)
# and will use up to address 23
dmx.add_device(DMX_UNIVERSE, DmxDevice("light_5", "power_lights", 6, 1))

# This device will be assigned start DMX address 31 (6 from the previous one, + 1)
# and will use up to address 30
dmx.add_device(DMX_UNIVERSE, DmxDevice("light_6", "power_lights", 6, 1))

# Value mapping is: [red, green, blue, white, amber, uv]
dmx.set_output_by_type(DMX_UNIVERSE, "power_lights", [0, 0, 0, 0, 0, 0])

dmx.send_update(DMX_UNIVERSE)

all_lights = ["light_1","light_2","light_3","light_4","light_5","light_6"]
duration = 2
print("red")
dmx.smooth_fade(DMX_UNIVERSE, all_lights, [0, 0, 0, 0, 0, 0], [255, 0, 0, 0, 0, 0], duration)

print("green")
dmx.smooth_fade(DMX_UNIVERSE, all_lights, [255, 0, 0, 0, 0, 0], [0, 255, 0, 0, 0, 0], duration)

print("blue")
dmx.smooth_fade(DMX_UNIVERSE, all_lights, [0, 255, 0, 0, 0, 0], [0, 0, 255, 0, 0, 0], duration)

print("uv")
dmx.smooth_fade(DMX_UNIVERSE, all_lights, [0, 0, 255, 0, 0, 0], [0, 0, 0, 0, 0, 255], duration)

print("black")
dmx.smooth_fade(DMX_UNIVERSE, all_lights, [0, 0, 0, 0, 0, 255], [0, 0, 0, 0, 0, 0], duration)


print("red")
dmx.smooth_fade(DMX_UNIVERSE, ["light_1"], [0, 0, 0, 0, 0, 0], [255, 0, 0, 0, 0, 0], duration)

print("green")
dmx.smooth_fade(DMX_UNIVERSE, ["light_2"], [0, 0, 0, 0, 0, 0], [0, 255, 0, 0, 0, 0], duration)

print("blue")
dmx.smooth_fade(DMX_UNIVERSE, ["light_3"], [0, 0, 0, 0, 0, 0], [0, 0, 255, 0, 0, 0], duration)

print("red")
dmx.smooth_fade(DMX_UNIVERSE, ["light_4"], [0, 0, 0, 0, 0, 0], [255, 0, 0, 0, 0, 0], duration)

print("green")
dmx.smooth_fade(DMX_UNIVERSE, ["light_5"], [0, 0, 0, 0, 0, 0], [0, 255, 0, 0, 0, 0], duration)

print("blue")
dmx.smooth_fade(DMX_UNIVERSE, ["light_6"], [0, 0, 0, 0, 0, 0], [0, 0, 255, 0, 0, 0], duration)


print("black")
dmx.smooth_fade(DMX_UNIVERSE, ["light_1"], [255, 0, 0, 0, 0, 0], [0, 0, 0, 0, 0, 0], duration)

print("black")
dmx.smooth_fade(DMX_UNIVERSE, ["light_2"], [0, 255, 0, 0, 0, 0], [0, 0, 0, 0, 0, 0], duration)

print("black")
dmx.smooth_fade(DMX_UNIVERSE, ["light_3"], [0, 0, 255, 0, 0, 0], [0, 0, 0, 0, 0, 0], duration)

print("black")
dmx.smooth_fade(DMX_UNIVERSE, ["light_4"], [255, 0, 0, 0, 0, 0], [0, 0, 0, 0, 0, 0], duration)

print("black")
dmx.smooth_fade(DMX_UNIVERSE, ["light_5"], [0, 255, 0, 0, 0, 0], [0, 0, 0, 0, 0, 0], duration)

print("black")
dmx.smooth_fade(DMX_UNIVERSE, ["light_6"], [0, 0, 255, 0, 0, 0], [0, 0, 0, 0, 0, 0], duration)