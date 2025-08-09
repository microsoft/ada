import time
from datetime import datetime, timedelta, timezone
from typing import List, Tuple

from suntime import Sun  # type: ignore


class States:
    INITIAL = "initial"
    OFF = "off"
    COOL_DOWN = "cool_down"
    CUSTOM = "custom"
    REBOOT = "reboot"
    ON = "on"
    ALL = [OFF, COOL_DOWN, CUSTOM, REBOOT, ON]


class StateMachine:
    def __init__(
        self,
        on_days: List[str],
        off_time: Tuple[int] | str,  # a hour/minute or "sunrise"/"sunset"
        on_time: Tuple[int] | str,  # a hour/minute or "sunrise"/"sunset"
        latitude: float,  # for resolving sunrise/sunset
        longitude: float,
        utc_offset: int,  # timezone offset in hours
        turn_off_timeout: int,  # in seconds
        custom_timeout: int,  # in seconds
        reboot_timeout: int,  # in seconds
    ):
        self.latitude = latitude
        self.longitude = longitude
        self.timezone = timezone(timedelta(hours=utc_offset))
        self.on_days = on_days
        self.off_time = off_time
        self.on_time = on_time
        self.turn_off_timeout = turn_off_timeout
        self.custom_timeout = custom_timeout
        self.reboot_timeout = reboot_timeout
        self.state = States.INITIAL
        self.turn_off_start = 0.0  # seconds
        self.custom_start = 0.0  # seconds since epoch
        self.reboot_start = 0.0  # seconds since epoch
        self.rebooting = (
            False  # performs auto-cooldown, then off for reboot timeout then on again.
        )
        self.power_off_day_of_week = ""  # used to track if we are off today

    def advance(self, now: float) -> str:
        desired_state = self._get_master_power_state(now)

        if self.state == States.INITIAL:
            raise ValueError(
                "StateMachine is in INITIAL state, cannot advance. "
                + "Please call set_state() once to communicate actual state of the lights."
            )

        if self.state == States.COOL_DOWN:
            if now - self.turn_off_start > self.turn_off_timeout:
                self.set_state(States.OFF, now)
                if self.rebooting:
                    # if we were rebooting, we need to turn back on after the reboot timeout
                    self.set_state(States.OFF, now)
                    self.reboot_start = now
        elif self.state == States.OFF and self.rebooting:
            if now - self.reboot_start > self.reboot_timeout:
                # if we were rebooting, we need to turn back on after the reboot timeout
                self.set_state(States.CUSTOM, now)
                self.rebooting = False
        elif self.state == States.CUSTOM:
            if now - self.custom_start > self.custom_timeout:
                self.set_state(States.ON if desired_state else States.OFF, now)

        elif self.state == States.OFF:
            if desired_state:
                self.set_state(States.ON, now)
        elif self.state == States.ON:
            if not desired_state:
                self.set_state(States.OFF, now)

        return self.state

    def turn_off(self, now: float = time.time()):
        # override the on time for the rest of the day.
        self.power_off_day_of_week = datetime.fromtimestamp(now).strftime("%A")
        self.set_state(States.OFF, now)

    def set_state(self, new_state, now: float = time.time()) -> str:
        if new_state in States.ALL:
            if new_state == States.OFF and self.state in [States.ON, States.CUSTOM]:
                # need to cool down before turning off
                new_state = States.COOL_DOWN
                self.turn_off_start = now
            elif new_state == States.CUSTOM:
                self.power_off_day_of_week = ""
                self.custom_start = now
            elif new_state == States.REBOOT:
                self.power_off_day_of_week = ""
                self.reboot_start = now
                self.rebooting = True
                new_state = States.COOL_DOWN
                self.turn_off_start = now
            elif new_state == States.ON:                
                self.power_off_day_of_week = ""

            self.state = new_state
            return new_state
        else:
            raise ValueError("Invalid state")

    def _get_master_power_state(self, now: float):
        if not self._get_on_today(now):
            return False
        on_hour, on_minute = self._resolve_sunrise_sunset(self.on_time)
        off_hour, off_minute = self._resolve_sunrise_sunset(self.off_time)
        d = datetime.fromtimestamp(now)
        if (d.hour > on_hour or (d.hour == on_hour and d.minute >= on_minute)) and (
            d.hour < off_hour or (d.hour == off_hour and d.minute < off_minute)
        ):
            return True
        return False

    def _get_on_today(self, now: float):
        """Check if Ada is scheduled to be on today and if so at what start and stop times.
        Sets the variables self.on_today, self.on_time, and self.off_time."""
        run_days = self.on_days
        today = datetime.fromtimestamp(now)
        day_of_week = today.strftime("%A")
        if self.power_off_day_of_week == day_of_week:
            # if we turned off today, we are not on today
            return False
        else:
            self.power_off_day_of_week = ""  # reset for next day
        return day_of_week in run_days

    def _resolve_sunrise_sunset(self, setting):
        sun = Sun(self.latitude, self.longitude)
        sunset = sun.get_sunset_time(time_zone=self.timezone)
        sunrise = sun.get_sunrise_time(time_zone=self.timezone)
        if setting == "sunrise":
            return [sunrise.hour, sunrise.minute]
        if setting == "sunset":
            return [sunset.hour, sunset.minute]
        return setting


def test():
    latitude = 47.67399000
    longitude = -122.12151000
    timezone = -7
    machine = StateMachine(
        ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday"],
        "sunset",
        "sunrise",
        latitude,
        longitude,
        timezone,
        60,
        30,
        10,
    )

    # pick a Friday so we roll into the weekend
    date = datetime(2025, 8, 1, 12, 0, 0, 0)
    now = date.timestamp()
    machine.set_state(States.OFF)
    custom = False
    reboot = False
    current = date.day
    # simulate 5 days we should see two days off for the weekend then on again on Monday
    for i in range(24 * 5):
        now += 3600
        date = datetime.fromtimestamp(now)
        if date.day != current:
            current = date.day
            print(
                f"New day: {date}======================================================"
            )
        state = machine.advance(now)  # Advance by 1 hour
        print(f"{date}: {state}")
        if state == States.COOL_DOWN:
            # make sure cool down state latches for turn off timeout
            while state == States.COOL_DOWN:
                now += 10
                state = machine.advance(now)  # Advance by 10 seconds
                print(f"{date}: {state} (cool down)")

        elif state == States.ON and not custom:
            custom = True
            state = States.CUSTOM
            machine.set_state(States.CUSTOM, now)
            print(f"{date}: {state} (custom)")
            # make sure custom state latches for custom timeout
            while state == States.CUSTOM:
                now += 10
                state = machine.advance(now)  # Advance by 10 seconds
                print(f"{date}: {state} (custom)")

        elif state == States.ON and not reboot:
            reboot = True
            state = States.REBOOT
            machine.set_state(States.REBOOT, now)
            print(f"{date}: {state} (reboot)")
            # make sure reboot state latches for cool down timeout + reboot timeout
            while state in [States.COOL_DOWN, States.OFF, States.REBOOT, States.CUSTOM]:
                now += 10
                state = machine.advance(now)  # Advance by 10 seconds
                print(f"{date}: {state} (reboot)")

            # test we can now turn off for the rest of the day.
            machine.turn_off(now)


if __name__ == "__main__":
    test()
