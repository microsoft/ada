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
        # performs auto-cooldown, then off for reboot timeout then on again.
        self.rebooting = False
        self.freeze_until = None  # freeze state (off or on) until this time is reached.

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

    def turn_on(self, now: float = time.time()):
        # override the on time for the rest of the day until next scheduled off time.
        self.freeze_until = self._get_freeze_time(States.ON, now)
        self.set_state(States.ON, now)

    def turn_off(self, now: float = time.time()):
        # override the off time for the rest of the day until next scheduled on time.
        self.freeze_until = self._get_freeze_time(States.OFF, now)
        self.set_state(States.OFF, now)

    def reset(self, now: float = time.time()):
        # reset any custom overrides
        self.freeze_until = None
        self.advance(now)

    def set_state(self, new_state: str, now: float = time.time()) -> str:
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

    def _get_freeze_time(self, state: str, now: float):
        on_hour, on_minute = self._resolve_sunrise_sunset(self.on_time)
        off_hour, off_minute = self._resolve_sunrise_sunset(self.off_time)
        d = datetime.fromtimestamp(now, tz=self.timezone)
        start = datetime(
            d.year, d.month, d.day, on_hour, on_minute, 0, tzinfo=self.timezone
        )
        end = datetime(
            d.year, d.month, d.day, off_hour, off_minute, 0, tzinfo=self.timezone
        )

        if d > start and d < end:
            # we are inside the on-time window, so freeze until the normal scheduled off time.
            return end
        else:
            # We are off so freeze until the normal scheduled on_time (possibly tomorrow since
            # off time normally wraps around to the next day).
            return start if d.hour < on_hour else start + timedelta(days=1)

    def _get_master_power_state(self, now: float):
        d = datetime.fromtimestamp(now, tz=self.timezone)
        on_hour, on_minute = self._resolve_sunrise_sunset(self.on_time)
        off_hour, off_minute = self._resolve_sunrise_sunset(self.off_time)
        start = datetime(
            d.year, d.month, d.day, on_hour, on_minute, 0, tzinfo=self.timezone
        )
        end = datetime(
            d.year, d.month, d.day, off_hour, off_minute, 0, tzinfo=self.timezone
        )

        if self.freeze_until is not None and d < self.freeze_until:
            # then we are in a power override state due to user pressing On or Off, and so we
            # stay in this state until the freeze time is over.
            return self.state in [States.ON, States.CUSTOM]

        if not self._get_on_today(now):
            return False

        return d >= start and d < end

    def _get_on_today(self, now: float):
        """Check if Ada is scheduled to be on today and if so at what start and stop times.
        Sets the variables self.on_today, self.on_time, and self.off_time."""
        run_days = self.on_days
        today = datetime.fromtimestamp(now)
        day_of_week = today.strftime("%A")
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
    start = date
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
        expecting_cool_down = False
        print(f"{date}: {state}")
        if state == States.COOL_DOWN:
            expecting_cool_down = True

        elif state == States.ON and not custom:
            custom = True
            state = States.CUSTOM
            machine.set_state(States.CUSTOM, now)
            print(f"{date}: {state} (custom)")

        elif state == States.ON and not reboot:
            reboot = True
            state = States.REBOOT
            machine.set_state(States.REBOOT, now)
            print(f"{date}: {state} (reboot)")
            expecting_cool_down = True
            state = machine.advance(now)

        elif date.hour == 21 and (date - start).days == 1:
            # test we can now turn off for the rest of the day.
            print(f"{date}: {state} (on override)")
            machine.turn_on(now)

        elif date.hour == 15 and (date - start).days == 4:
            # test we can now turn off for the rest of the day.
            print(f"{date}: {state} (off override)")
            machine.turn_off(now)
            expecting_cool_down = True
            state = machine.state

        if expecting_cool_down:
            expecting_cool_down = False
            # make sure reboot state latches for cool down timeout + reboot timeout
            while state in [States.COOL_DOWN, States.REBOOT, States.CUSTOM]:
                now += 10
                date = datetime.fromtimestamp(now)
                state = machine.advance(now)  # Advance by 10 seconds
                print(f"{date}: {state}")


if __name__ == "__main__":
    test()
