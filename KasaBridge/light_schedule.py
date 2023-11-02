# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
import argparse
from tplink_smartplug import *
import datetime
from astral import Astral


class Schedule:
    def __init__(self):
        self.lights = []

    def add_light(self, addr):
        self.lights += [TplinkSmartPlug(addr)]

    def turn_off(self):
        for i in self.lights:
            i.turn_off()

    def turn_on(self):
        for i in self.lights:
            i.turn_on()

    def run(self):
        # we want to turn off at midnight and on at sunset.
        # the 'time_to_sunset' is negative if it is time to turn them on,
        # otherwise it is positive.

        while True:
            now = datetime.datetime.now()
            # now = datetime.datetime(2020,1,8,22,0,0)
            midnight = datetime.datetime(
                now.year, now.month, now.day, 0, 0, 0
            ) + datetime.timedelta(days=1)

            a = Astral()
            city = a["Seattle"]
            sun = city.sun(date=now, local=True)
            sunset = sun["sunset"]

            time_to_sunset = sunset.replace(tzinfo=None) - now
            time_to_midnight = midnight - now

            print(
                "turning on in {} seconds".format(int(time_to_sunset.total_seconds())),
                flush=True,
            )
            print(
                "turning off in {} seconds".format(
                    int(time_to_midnight.total_seconds())
                ),
                flush=True,
            )

            if time_to_sunset.days < 0:
                print("turning on", flush=True)
                self.turn_on()
                seconds = int(time_to_midnight.total_seconds()) + 1
            else:
                print("turning off", flush=True)
                self.turn_off()
                seconds = int(time_to_sunset.total_seconds()) + 1

            print("sleeping for {} seconds".format(seconds), flush=True)
            time.sleep(seconds)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        "light_schedule turns lights on at sunset and off at midnight"
    )
    parser.add_argument(
        "--ip", help="one or more ip addresses of HS105 devices", nargs="+"
    )
    args = parser.parse_args()

    if args.ip is None:
        print("Please specify --ip option", flush=True)
    else:
        s = Schedule()
        for ip in args.ip:
            s.add_light(ip)
        s.run()
