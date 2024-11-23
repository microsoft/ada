# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
import _thread
import os
import subprocess
import time
from pathlib import Path


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


def find_program(name):
    """Find the given program in the PATH and return the full path to it"""
    extensions = [] if os.name == "posix" else [".cmd", ".bat", ".exe"]
    for path in os.environ["PATH"].split(os.pathsep):
        for ext in extensions:
            exe_file = os.path.join(path, name + ext)
            if os.path.isfile(exe_file):
                return exe_file
    raise Exception(f"Could not find program '{name}' in PATH")


class Process:
    def __init__(self, cmd_line: str, cwd: str, log_file: str):
        """Run subprocess and send the output to the given log file."""
        args = cmd_line.split(" ")
        cmd = args[0]
        args = args[1:]
        if not cwd.startswith("/") and cmd[1] != ":":
            os_cwd = os.getcwd()
            resolved = Path(os_cwd) / Path(cwd)
            cwd = os.path.realpath(str(resolved))
        cmd = find_program(cmd)

        self.proc = subprocess.Popen(
            [cmd] + args,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            encoding="utf-8",
            cwd=cwd,
        )

        self.closed = False
        self.output = open(log_file, "a")

        if self.proc.stdout is not None:
            _thread.start_new_thread(self.write_output, (self.proc.stdout,))

        if self.proc.stderr is not None:
            _thread.start_new_thread(self.write_output, (self.proc.stderr,))

    def terminate(self):
        self.closed = True
        self.proc.terminate()
        self.output.close()

    def write_output(self, file):
        for line in file:
            if self.closed:
                break
            self.output.write(line)
            self.output.flush()
        print("write_output terminating")
