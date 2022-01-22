from dmx import DmxDevice
from pretty_print import print_info


class Dmx:
    def __init__(self, port_name: str):
        self.port_name = port_name

    def open_serial(self):
        pass

    def try_reconnect(self):
        print_info("DMX: Re-opening port")

    def send_data(self, label: int, data=bytes()):
        pass

    def receive_data(self, label):
        return None

    def set_api_key(self):
        pass

    def enable_dmx_ports(self, port_1_enable, port_2_enable):
        pass

    def get_serial_number(self):
        return 0

    def get_hardware_version(self):
        return 0

    def add_device(self, dmx_universe: int, dmx_device: DmxDevice):
        pass

    def set_output_by_type(self, dmx_universe: int, device_type: str, output_values):
        pass

    def set_output_by_name(self, dmx_universe: int, device_name: str, output_values):
        pass

    def send_update(self, dmx_universe: int):
        pass

    # smoothly fades each light, where there is an old and new color for each light to send
    def smooth_fade(self, universe, lights, oldcolors, newcolors, seconds):
        pass
