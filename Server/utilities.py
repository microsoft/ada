# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
import time


class TimedLatch:
    """
    This class provides a timed latch, meaning a switch that triggers no more frequently
    than the given delay.  This can be used to smooth a signal.  The is_on method is also
    a latch that resets automatically so it only returns True once.  Then it will not return
    True again until the delay is reached.
    """
    def __init__(self, delay=10):
        self.changed = time.time()
        self.delay = delay
        self.on = False
        self.switch_time = time.time() - delay - 1

    def is_on(self):
        if self.on:
            # toggle the latch, return True once and then wait for timeout before
            # allowing it to turn on again.
            self.on = False
            return True
        return False

    def switch(self, new_value):
        changed = False
        if new_value:
            if self.switch_time + self.delay < time.time():
                # delay has elapsed, so it is ok to turn it on again.
                self.switch_time = time.time()
                self.on = True
                changed = True
        return changed
