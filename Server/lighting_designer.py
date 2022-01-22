import datetime
import json
import random
import time
import numpy as np
from threading import Thread
from collections import Counter
from suntime import Sun
from utilities import TimedLatch
from animation import AnimationLoop
from priority_queue import PriorityQueue


class LightingDesigner:
    """
    This class manages the combination of various effects that we want to play
    using various settings from the given config file.
    It can also manage the "lights off" sequence that happens
    at the end of the day forcing all lights to go dark and new animations be forgotten
    until the power comes back on the next day, both on the RaspberryPi's and on the DMX controller.
    """
    def __init__(self, server, msgbus, sensei, config):
        self.config = config
        self.msgbus = msgbus
        msgbus.add_listener(self.onmessage)
        self.msgqueue = PriorityQueue()
        self.lights_on = None
        self.server = server
        self.sensei = sensei
        self.bridge = None
        self.overlay = None
        self.running = False
        self.thread = None
        self.zone_maps = {}
        self.enable_movement = True
        self.movement_latch = TimedLatch(10)
        self.core_is_on = False
        self.last_change = time.time()
        self.last_cool_animation = time.time()
        self.last_color = None
        self.last_camera_emotion = time.time()
        self.turn_off_time = None
        self.animations = None
        self.power_on_override = None
        self.power_off_override = None
        self.color_override = None
        self.is_raining = None
        self.stop_rain_time = None
        self.on_time = self.resolve_sunrise_sunset(self.config.on_time)
        self.off_time = self.resolve_sunrise_sunset(self.config.off_time)
        if self._get_cool_animation_time():
            # if server is started when we are already into cool animation time, so ensure this happens.
            self.last_cool_animation = 0

    def update_tplink_status(self):
        if not self.bridge:
            return

        result = self.send_bridge_command("status")
        if result:
            parts = result.split(',')
            states = []
            for device in parts:
                pair = device.split(":")
                if len(pair) == 2:
                    is_on = pair[1] == "True"
                    print("tplink device at {} is {}".format(pair[0], is_on))
                    states += [is_on]
            if np.all(states):
                self.lights_on = True
            elif np.all([not x for x in states]):
                self.lights_on = False
            else:
                # mixed states needs to be corrected later
                self.lights_on = None

    def start(self):
        if not self.running:
            self.running = True
            self.choreographer = Thread(target=self._choreographer_thread)
            self.choreographer.daemon = True
            self.choreographer.start()
            self.sensei.start()

    def stop(self):
        self.running = False
        if self.choreographer is not None and self.choreographer.is_alive():
            self.choreographer.join(timeout=1)
            self.choreographer = None
        self.sensei.stop()

    def _fade_to_black(self):
        """ Turn off lights to pi and DMX by fading to black. """
        self.server.clear_queue()
        self.is_raining = False
        self.server.queue_command(0, [{"command": "StopRain"},
                                      {"command": "sensei", "seconds": 2, "colors": [[0, 0, 0]]}])

    def send_bridge_command(self, command):
        try:
            return self.bridge.send_command(command)
        except:
            self.server.on_bridge_error()
            self.bridge = None
            return "error"

    def _master_power_on(self):
        # self.server.insights.track_power(1)
        print("### turning on the lights...", end='', flush=True)
        if self.bridge:
            response = self.send_bridge_command("on")
            print("{}".format(response))
        self.msgbus.send('server', '/power/on')
        self.lights_on = True
        self.server.camera_on()

    def _master_power_off(self):
        # self.server.insights.track_power(0)
        print("### turning off the lights...", end='', flush=True)
        if self.bridge:
            response = self.send_bridge_command("off")
            print("{}".format(response))
        self.msgbus.send('server', '/power/off')
        self.lights_on = False
        self.server.camera_off()

    def resolve_sunrise_sunset(self, setting):
        latitude = self.config.latitude
        longitude = self.config.longitude
        sun = Sun(latitude, longitude)
        sunset = sun.get_local_sunset_time()
        sunrise = sun.get_local_sunrise_time()
        if setting == "sunrise":
            return [sunrise.hour, sunrise.minute]
        if setting == "sunset":
            return [sunset.hour, sunset.minute]
        return setting

    def _find_animation(self, name):
        for index in range(len(self.config.cool_animations)):
            animation = self.config.cool_animations[index]
            if animation["Name"] == name:
                return animation
        return None

    def onmessage(self, user, msg):
        if user == 'server':
            # ignore our own messages.
            return
        self.msgqueue.enqueue(0, msg)

    def process_next_message(self):
        item = self.msgqueue.dequeue()
        if item is None:
            return

        _, msg = item
        parts = msg[1:].split("/")
        if len(parts) < 2:
            print("### ignoring message:", msg)
        else:
            cmd = parts[0]
            option = parts[1]
            # every remote command turns ada on, except for the "run" command which
            # puts ada back on program according to config.
            if cmd == "power":
                if option == "on":
                    print("### power on override")
                    self._master_power_on()
                    self.power_on_override = True
                    self.power_off_override = False
                elif option == "off":
                    print("### power off override")
                    self.power_on_override = False
                    self.power_off_override = True
                    self.color_override = False
                    self.animations = None
                    self._fade_to_black()
                    self.turn_off_time = time.time() + self.config.turn_off_timeout
                elif option == "run":
                    print("### back to normal operation")
                    self.power_on_override = False
                    self.power_off_override = False
                    self.color_override = False
                    self.turn_off_time = None
                    self.animations = None
                    self.msgbus.send('server', '/power/run')
                else:
                    print("error: invalid power command: {}".format(msg))
            elif cmd == "rain":
                if option == "toggle":
                    if self.is_raining:
                        option = "off"
                    else:
                        option = "on"
                if option == "on":
                    self.power_on_override = True
                    self.power_off_override = False
                    self.server.queue_command(0, {"command": "StartRain", "size": 12, "amount": 50.0})
                    self.is_raining = True
                    self.stop_rain_time = None
                elif option == "off":
                    self.server.queue_command(0, {"command": "StopRain"})
                    self.is_raining = False
                    self.stop_rain_time = None
                else:
                    print("error: invalid rain command: {}".format(msg))

            elif cmd == "animation":
                try:
                    animation = self._find_animation(option)
                    if animation is not None:
                        self.power_on_override = True
                        self.power_off_override = False
                        self.color_override = False
                        self.animations = AnimationLoop()
                        self.animations.start(animation)
                except:
                    print("### invalid animation requested: {}".format(msg))
            elif cmd == "emotion":
                print("### emotion override: {}".format(option))
                self._blush(None, option, seconds=2, hold=2)
                self.color_override = True
                self.animations = None
                self.power_on_override = True
                self.power_off_override = False
            elif cmd == "color":
                print("### color override: {}".format(option))
                rgb_color = [int(x) for x in option.split(',')]
                self._set_color(None, rgb_color)
                self.color_override = True
                self.animations = None
                self.power_on_override = True
                self.power_off_override = False
            elif cmd == "dmx":
                print("### dmx color override: {}".format(option))
                colors = []
                i = 1
                while i < len(parts):
                    color = [int(x) for x in parts[i].split(',')]
                    if len(color) == 3:
                        colors += [color]
                    i += 1
                if len(colors) > 0:
                    self._set_dmx(colors)
                    self.color_override = True
                    self.animations = None
                    self.power_on_override = True
                    self.power_off_override = False
            elif cmd == "zone":
                zone = int(option)
                if len(parts) == 3:
                    color = [int(x) for x in parts[2].split(',')]
                    if len(color) == 3:
                        print("### zone {} color: {}".format(zone, color))
                        self._set_zone(zone, color)
                        self.color_override = True
                        self.animations = None
                        self.power_on_override = True
                        self.power_off_override = False
            elif cmd == "strip":
                target = option
                if len(parts) == 4:
                    strip = int(parts[2])
                    color = [int(x) for x in parts[3].split(',')]
                    if len(color) == 3:
                        print("### strip {} {} color: {}".format(target, strip, color))
                        self._set_strip(target, strip, color)
                        self.color_override = True
                        self.animations = None
                        self.power_on_override = True
                        self.power_off_override = False
            elif cmd == "gradient":
                target = option
                if len(parts) > 4:
                    strip = -1 if parts[2] == '' else int(parts[2])
                    colorsPerStrip = -1 if parts[3] == '' else int(parts[3])
                    seconds = int(parts[4])
                    i = 5
                    colors = []
                    while i < len(parts):
                        color = [int(x) for x in parts[i].split(',')]
                        if len(color) == 3:
                            colors += [color]
                        i += 1
                    if len(colors) > 0:
                        print("### gradient {} {} colors: {}".format(target, strip, colors))
                        self._add_gradient(target, strip, colorsPerStrip, colors, seconds)
                        self.color_override = True
                        self.animations = None
                        self.power_on_override = True
                        self.power_off_override = False
            elif cmd == "pixels":
                self._set_pixels(msg)

    def _set_pixels(self, cmd):
        s = cmd.split('/')
        targets = {}
        # pimap converts numeric index back into actual adapi1, adapi2, adapi3 target names.
        pimap = {}
        index = 0
        for k in self.config.zone_maps:
            pimap[index] = k
            index += 1

        if len(s) == 6:
            _, t, strip, ledranges, color = s[1:]
            pixel = color.split(',')
            if len(pixel) == 3:
                r, g, b = pixel
                t = int(t)
                if t in pimap:
                    target = pimap[t]
                    if target not in targets:
                        targets[target] = []
                    pixels = targets[target]
                    color = [int(r), int(g), int(b)]
                    pixels += [{"s": int(strip), "l": ledranges, "color": color}]

        for k in targets:
            pixels = targets[k]
            cmd = {"command": "SetPixels", "target": target, "pixels": pixels}
            self.server.queue_command(1, [cmd])

        self.last_change = time.time()

    def _choreographer_thread(self):
        self.rainbow_timeout = time.time()
        next_sunrise_check = time.time()
        while self.running:
            self.server.increment()
            if time.time() >= next_sunrise_check:
                self.on_time = self.resolve_sunrise_sunset(self.config.on_time)
                self.off_time = self.resolve_sunrise_sunset(self.config.off_time)
                next_sunrise_check = time.time() + 3600

            bridge = self.server.get_bridge()
            if bridge != self.bridge:
                self.bridge = bridge
                self.update_tplink_status()
            new_clients = self.server.get_stale_clients()
            has_new_clients = len(new_clients) > 0
            wc = self.server.get_web_server_context()
            web_cmd = wc.commands.dequeue()
            if web_cmd is not None:
                _, cmd = web_cmd
                self.onmessage("web", cmd)

            self.msgbus.check_connection()
            try:
                self.process_next_message()
            except:
                pass

            # highest priority is the master power schedule
            power_state = self._get_master_power_state()
            if power_state or self.power_on_override:
                if self.lights_on is None or not self.lights_on:
                    self._master_power_on()
                    # if we just did a power cycle then reset any previous color overrides.
                    self.color_override = False
                    self.animations = None
                    print("### back to normal operation")
                    wc.run = None
                    self.turn_off_time = None
                    self.sensei.start()
                    self.msgbus.send('server', '/power/run')
                self.server.camera_on()
            elif not power_state or self.power_off_override:
                if self.lights_on is None or self.lights_on or has_new_clients:
                    if self.turn_off_time is None or has_new_clients:
                        print("### cooling down for {} seconds".format(self.config.turn_off_timeout))
                        self.turn_off_time = time.time() + self.config.turn_off_timeout
                        self._fade_to_black()
                        self.color_override = False
                        self.animations = None
                        continue
                    elif time.time() > self.turn_off_time:
                        self._master_power_off()
                        self.color_override = False
                        self.animations = None
                        self.turn_off_time = None
                        self.sensei.stop()  # no need to keep pinging cosmos while we are sleeping.
                        on_hour, on_minute = self.on_time
                        print("### lights are off, waiting for wake up time: {}:{}".format(
                            on_hour, on_minute))
                    else:
                        # let the lights cool down for 3 minutes before turning them off.
                        time.sleep(1)
                        continue

                self.server.camera_off()
                time.sleep(1)
                self.server.clear_queue()
                continue  # wait for power to go on again tomorrow.

            if self.stop_rain_time is not None and time.time() > self.stop_rain_time:
                self.stop_rain_time = None
                self.server.queue_command(0, {"command": "StopRain", "seconds": 1})
                self.is_raining = False

            if not self.color_override and self.animations is not None:
                if has_new_clients:
                    # make sure animation is not finished.
                    self.animations.reset()
                if self.animations.ready():
                    commands = self.animations.next()
                    if commands is not None:
                        for c in commands:
                            cmd = c["command"]
                            if cmd == "StartRain":
                                self.is_raining = True
                            elif cmd == "StopRain":
                                self.is_raining = False
                        self.server.clear_queue()
                        self.server.queue_command(0, commands)
                if self.animations.completed():
                    self.animations = None
                time.sleep(0.025)
                continue

            if self.rainbow_timeout > time.time() and not has_new_clients:
                # let the magical rainbow play for it's full duration with no interruptions.
                # unless we got new raspberry pi clients in which case we have to
                # drop through to send it the current animation.
                time.sleep(0.025)
                continue

            # highest priority is movement under the camera.
            camera_action = self.server.read_camera(1)
            if not self.color_override and camera_action is not None:
                priority, camera_action = camera_action
                parts = camera_action.split(' ')
                cmd = parts[0]
                if cmd == "emotions:":
                    if len(parts) > 1:
                        emotions = parts[1]
                        self._handle_camera_emotions(emotions)
                        continue
                elif cmd == "faces:":
                    # face count
                    if len(parts) > 1:
                        count = int(parts[1])
                        # self.server.insights.track_ada_faces(count)
                        if count > 2:
                            print("### time for some fun!")
                            self.core_is_on = True
                            seconds = self.config.rainbow_timeout
                            self.server.queue_command(0, {"command": "Rainbow", "seconds": 0, "hold": seconds,
                                                      "length": 157})
                            self.last_change = time.time()
                            self.rainbow_timeout = time.time() + seconds
                            continue
                elif cmd == "movement:":
                    # acknowledge movement under the camera by starting the rain overlay immediately, on
                    # top of any existing slow fade animation.
                    if len(parts) > 1:
                        movement = parts[1]
                        # self.server.insights.track_ada_movement(1 if movement == "movement" else 0)
                        if self.enable_movement:
                            if self.movement_latch.switch(movement == "movement"):
                                print("### movement detected!")
                                # enqueue a new start rain command (but only if it is not already raining!)
                                if self.stop_rain_time is None or self.stop_rain_time < time.time():
                                    self.server.queue_command(0, {"command": "StartRain", "size": 12, "amount": 50.0})
                                self.stop_rain_time = time.time() + self.config.movement_rain_timeout
                                self.last_change = time.time()

            if not self.color_override and self._get_cool_animation_time():
                self.last_change = time.time()
                if self.last_cool_animation + self.config.cool_animation_timeout > time.time() and not has_new_clients:
                    continue
                num = len(self.config.cool_animations) - 1
                i = random.randint(0, num)
                animation = self.config.cool_animations[i]
                self.animations = AnimationLoop()
                self.animations.start(animation, self.config.cool_animation_timeout)
                self.last_cool_animation = time.time()
                continue

            new_emotions = self.sensei.get_next_emotions()
            if not self.color_override and new_emotions is not None:
                print("Got new emotions: {}".format(new_emotions))
                # if not self.sensei.play_recording:
                #    self.server.insights.track_new_emotions(new_emotions)
                self._handle_new_emotions(new_emotions)
                continue

            if has_new_clients:
                if self.last_color is None:
                    self.last_color = [0, 0, 0]
                print("### Clients {} are out of date, replaying the last color".format(new_clients))
                for target in new_clients:
                    self._set_color(target, self.last_color)

            time.sleep(0.025)

    def _handle_camera_emotions(self, emotions):
        # IpCameraGui could send something like this, depending on how many faces are looking at the camera:
        # ['Happiness,0.9999596', 'Happiness,0.997026', 'Happiness,0.7164325', 'Happiness,0.703617']
        no_scores = [x.strip() for x in emotions.replace('[', '').replace(']', '').replace("'", "").split(",")][::2]
        # just show the most popular emotion.
        if len(no_scores) > 1:
            value, count = Counter(no_scores)
        elif len(no_scores) == 0:
            return
        else:
            value = no_scores[0]
        if value:
            print("### Highlighting camera emotion: {}".format(value))
            # self.server.insights.track_ada_emotions(value)
            self._blush(None, value, seconds=2, hold=self.config.hold_camera_blush, priority=5)

    def _get_master_power_state(self):
        on_hour, on_minute = self.on_time
        off_hour, off_minute = self.off_time
        now = datetime.datetime.now()
        if ((now.hour > on_hour or (now.hour == on_hour and now.minute >= on_minute)) and
           (now.hour < off_hour or (now.hour == off_hour and now.minute < off_minute))):
            return True
        return False

    def _get_cool_animation_time(self):
        if self.server.idle() and self.last_change + self.config.cool_animation_timeout < time.time():
            return True

        on_hour, on_minute = self.config.cool_animation_time
        now = datetime.datetime.now()
        if (now.hour > on_hour or (now.hour == on_hour and now.minute >= on_minute)):
            return True
        return False

    def _handle_new_emotions(self, emotions):
        for target in list(self.server.get_clients()):
            self._target_new_emotions(emotions, target)

    def _target_new_emotions(self, emotions, target):
        # Check if 3 or more zones have the same key:
        c = Counter(emotions)
        value, count = c.most_common()[0]
        if count >= 3:
            self._blush(target, value, hold=self.config.hold_sensei_blush, priority=10)

        # smooth fade to the new colors
        self._fade_to_zones(target, emotions, 10)

    def _blush(self, target, emotion, seconds=3, hold=10, priority=10):
        # blush all one color for 10 seconds
        if target == "DMX":
            return

        self.core_is_on = True
        rgb_color = self.config.colors_for_emotions[emotion]
        # hold the blush for 10 seconds
        self._set_color(target, rgb_color, seconds, hold, priority)

    def _set_color(self, target, rgb_color, seconds=2, hold=2, priority=10):
        self.last_color = rgb_color
        command = {"command": "CrossFade", "colors": [rgb_color], "seconds": seconds, "hold": hold}
        if target is not None:
            command["target"] = target
        print(command)
        self.server.queue_command(priority, command)
        self.last_change = time.time()

    def _set_dmx(self, rgb_colors, seconds=2, hold=2, priority=10):
        command = {"command": "sensei", "target": "DMX", "seconds": seconds, "colors": rgb_colors, "hold": hold}
        print(command)
        self.server.queue_command(priority, command)
        self.last_change = time.time()

    def _set_zone(self, zone, color, priority=10):
        columns = []
        for target in list(self.server.get_clients()):
            zone_map = self._get_zone_map(target)
            for row in zone_map["zone_leds"]:
                if row["zone"] == zone:
                    columns += [{"index": row["col"], "color": color}]
                for row in zone_map["core_leds"]:
                    columns += [{"index": row["col"], "color": color}]
            command = {"command": "ColumnFade", "target": target, "seconds": 0, "columns": columns}
            self.server.queue_command(1, [command])
        self.last_change = time.time()

    def _set_strip(self, target, strip, color, priority=10):
        commands = []
        columns = [{"index": strip, "color": color}]
        command = {"command": "ColumnFade", "target": target, "seconds": 0, "columns": columns}
        commands += [command]
        self.server.queue_command(1, commands)
        self.last_change = time.time()

    def _add_gradient(self, target, strip, colorsPerStrip, colors, seconds, priority=10):
        if target == "DMX":
            return  # not supported on dmx at the moment...
        command = {
            "command": "Gradient",
            "colors": colors,
            "seconds": seconds
        }
        if target and target != "*":
            command["target"] = target
        if strip != -1:
            command["strip"] = strip
        if colorsPerStrip != 0:
            command["cps"] = colorsPerStrip
        self.server.queue_command(priority, command)
        self.last_change = time.time()

    def _fade_to_zones(self, target, emotions, priority):
        command = None
        if target == "DMX":
            rgb_colors = self._get_rgb_colors(emotions, self.config.colors_for_dmx_emotions)
            # SET "light_1" and 2 to be dark blue
            rgb_colors[0] = [0, 0, 80]
            rgb_colors[1] = [0, 0, 80]
            command = {"command": "sensei", "target": target, "seconds": 2,
                       "colors": rgb_colors}
        else:
            rgb_colors = self._get_rgb_colors(emotions, self.config.colors_for_emotions)
            if len(rgb_colors) < len(self.config.camera_zones):
                print("### don't have enough colors???")

            # return core to just the optical fiber led's.
            strips_command = self._get_zone_color_command(target, rgb_colors, False)
            if self.core_is_on:
                self.core_is_on = False
                core_off_command = self._get_core_led_off_command(target)
                core_on_command = self._get_core_led_command(target)
                command = [core_off_command, core_on_command, strips_command]
            else:
                command = strips_command

        self.server.queue_command(priority, command)
        self.last_change = time.time()

    def _get_rgb_colors(self, emotions, colors_for_emotions):
        rgb_colors = []
        last_key = "Happiness"
        for key in emotions:
            last_key = key
            rgb_colors += [colors_for_emotions[key]]
        while len(rgb_colors) < len(self.config.camera_zones):
            rgb_colors += [colors_for_emotions[last_key]]
        return rgb_colors

    def _get_zone_map(self, target):
        """ return the zone_map for the given raspberry pi """
        filename = "zone_map_1.json"  # handy to return something while debugging.
        if target in self.config.zone_maps:
            filename = self.config.zone_maps[target]

        if filename not in self.zone_maps:
            with open(filename, 'r') as f:
                self.zone_maps[filename] = json.load(f)
        return self.zone_maps[filename]

    def _get_zone_color_command(self, target, sensei_colors, include_core, seconds=2):
        zone_map = self._get_zone_map(target)
        columns = []
        for i in range(len(sensei_colors)):
            color = sensei_colors[i]
            for row in zone_map["zone_leds"]:
                if row["zone"] == i:
                    columns += [{"index": row["col"], "color": color}]

        if include_core:
            color = sensei_colors[-1]  # the Ip camera color.
            # light up the center core as well.
            for row in zone_map["core_leds"]:
                columns += [{"index": row["col"], "color": color}]

        return {"command": "ColumnFade", "target": target, "seconds": seconds,
                "columns": columns}

    def _get_core_led_off_command(self, target):
        zone_map = self._get_zone_map(target)
        color = [0, 0, 0]
        columns = []
        for row in zone_map["core_leds"]:
            columns += [{"index": row["col"], "color": color}]

        return {"command": "ColumnFade", "target": target, "seconds": 2,
                "columns": columns}

    def _get_core_led_command(self, target):
        zone_map = self._get_zone_map(target)
        color = [255, 255, 255]
        pixels = []
        for row in zone_map["core_leds"]:
            strip = row["col"]
            leds = ",".join([str(x) for x in row["leds"]])
            pixels += [{"s": strip, "l": leds, "color": color}]
        return {"command": "SetPixels", "target": target, "pixels": pixels}