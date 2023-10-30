# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
"""Interface to Enttec DMX USB Pro MK2 Controller."""
import time
import serial

from pretty_print import print_info, print_error

HEADER_LENGTH = 4
START_CODE = 0x7E
END_CODE = 0xE7
API_KEY = bytes([0xC9, 0xA4, 0x03, 0xE4])

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


class DmxDevice:
    def __init__(
        self,
        device_name: str,
        device_type: str,
        output_channels: int,
        bytes_per_channel: int,
    ):
        self.device_name = device_name
        self.device_type = device_type
        self.output_channels = output_channels
        self.output_values = []
        if bytes_per_channel < 1:
            self.bytes_per_channel = 1
        elif bytes_per_channel > 2:
            self.bytes_per_channel = 2
        else:
            self.bytes_per_channel = bytes_per_channel

    def set_output(self, output_values):
        self.output_values = output_values

    def get_output(self):
        return self.output_values

    def get_dmx_byte_array(self):
        dmx_byte_array = bytearray(self.output_channels * self.bytes_per_channel)
        if self.bytes_per_channel == 1:
            for i, val in enumerate(self.output_values):
                dmx_byte_array[i] = val
        elif self.bytes_per_channel == 2:
            for i, val in enumerate(self.output_values):
                dmx_byte_array[i * 2] = (self.output_values[i] >> 8) & 0xFF
                dmx_byte_array[i * 2 + 1] = self.output_values[i] & 0xFF
        return dmx_byte_array


class Dmx:
    def __init__(self, port_name: str):
        self.port_name = port_name
        self.open_serial()
        self.universe_1 = list()
        self.universe_2 = list()

        # THIS ONLY WORKS WITH THE ENTTEC DMX USB PRO MK2
        # self.set_api_key()
        # time.sleep(0.2)
        # self.enable_dmx_ports(True, True)
        # time.sleep(0.2)
        # version = self.get_hardware_version()
        # if version is not None and version > 1:
        #     print_info("DMX: Controller found")
        # else:
        #     print_error('DMX: Controller not found')

    def open_serial(self):
        try:
            self.port = serial.Serial(self.port_name, baudrate=115200, timeout=1000)
            self.port.flushInput()
        except Exception as e:
            print_error("DMX: Unable to open serial port. \n %s" % e)

    def try_reconnect(self):
        print_info("DMX: Re-opening port")
        try:
            self.port.close()
        except:
            pass
        self.open_serial()

    def send_data(self, label: int, data=bytes()):
        try:
            # Write header
            length_lsb = len(data) & 0xFF
            length_msb = len(data) >> 8
            header = bytes([START_CODE, label, length_lsb, length_msb])
            self.port.write(header)
            # Write data
            if data:
                self.port.write(data)
            # Write endcode
            self.port.write(bytes([END_CODE]))
        except:
            print_error("DMX: Unable to send data")
            time.sleep(5)
            self.try_reconnect()

    def receive_data(self, label):
        try:
            # Read header
            header = self.port.read(HEADER_LENGTH)
            start_code = header[0]
            if start_code != START_CODE:
                print_error("DMX: Invalid return start code")
                return False
            read_label = header[1]
            if read_label != label:
                print_error("DMX: Unexpected return label")
                return False
            data_length = header[2]
            data_length = data_length + (header[3] << 8)
            # Read data
            data = self.port.read(data_length)
            # Read encode
            end_code = self.port.read(1)[0]
            if end_code != END_CODE:
                print_error("DMX: Invalid return end code " + str(end_code))
                return False
            return data
        except:
            print_error("DMX: Unable to receive data")

    def set_api_key(self):
        self.send_data(13, API_KEY)

    def enable_dmx_ports(self, port_1_enable, port_2_enable):
        enable = bytearray(2)
        if port_1_enable:
            enable[0] = 1
        if port_2_enable:
            enable[1] = 1
        self.send_data(147, enable)

    def get_serial_number(self):
        try:
            self.send_data(10)
            serial_num = self.receive_data(10)
            return serial_num
        except:
            print_error("DMX: Unable to get serial number")

    def get_hardware_version(self):
        try:
            self.send_data(14)
            version = self.receive_data(14)[0]
            return version
        except:
            print_error("DMX: Unable to get hardware version")

    def add_device(self, dmx_universe: int, dmx_device: DmxDevice):
        if dmx_universe == 1:
            self.universe_1.append(dmx_device)
        elif dmx_universe == 2:
            self.universe_2.append(dmx_device)

    def set_output_by_type(self, dmx_universe: int, device_type: str, output_values):
        if dmx_universe == 1:
            universe = self.universe_1
        else:
            universe = self.universe_2

        for device in universe:
            if device.device_type == device_type:
                device.set_output(output_values)

    def set_output_by_name(self, dmx_universe: int, device_name: str, output_values):
        if dmx_universe == 1:
            universe = self.universe_1
        else:
            universe = self.universe_2

        for device in universe:
            if device.device_name == device_name:
                device.set_output(output_values)

    def send_update(self, dmx_universe: int):
        if dmx_universe == 1:
            universe = self.universe_1
        else:
            universe = self.universe_2

        dmx_bytearray = bytearray(
            1
        )  # DMX payload needs to start with this empty byte at pos 0

        for dmx_device in universe:
            dmx_bytearray += dmx_device.get_dmx_byte_array()

        # Some devices ignore DMX endcode and expect 512 bytes
        padding = bytearray(512 - len(dmx_bytearray))
        dmx_bytearray += padding

        if dmx_universe == 1:
            self.send_data(6, dmx_bytearray)
        elif dmx_universe == 2:
            self.send_data(202, dmx_bytearray)

    # smoothly fades each light, where there is an old and new color for each light to send
    def smooth_fade(self, universe, lights, oldcolors, newcolors, seconds):
        start = time.time()
        while start + seconds > time.time():
            now = time.time()
            percent = (float(now) - float(start)) / seconds
            for i in range(len(lights)):
                name = lights[i]
                c1 = oldcolors
                if isinstance(c1, list):
                    c1 = oldcolors[i]
                c2 = newcolors
                if isinstance(c2, list):
                    c2 = newcolors[i]
                dr = c2[0] - c1[0]
                dg = c2[1] - c1[1]
                db = c2[2] - c1[2]
                da = c2[3] - c1[3]
                dw = c2[4] - c1[4]
                du = c2[5] - c1[5]
                r = int(c1[0] + float(dr) * percent)
                g = int(c1[1] + float(dg) * percent)
                b = int(c1[2] + float(db) * percent)
                a = int(c1[3] + float(da) * percent)
                w = int(c1[4] + float(dw) * percent)
                u = int(c1[5] + float(du) * percent)
                self.set_output_by_name(universe, name, [r, g, b, a, w, u])
            self.send_update(1)

    def findDMXDevices():
        import socket

        # listen for Art-Net broadcast
        artnet_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        artnet_sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

        artnet_sock.bind(("", 6454))

        while True:
            data, addr = artnet_sock.recvfrom(1024)  # buffer size is 1024 bytes
            if len(data) >= 7:
                header = data[:7].decode("utf8")
                if header == "Art-Net":
                    yield addr
