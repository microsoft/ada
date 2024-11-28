import _thread
import logging
import time

import coloredlogs
from paramiko.channel import Channel, ChannelFile

PREFIX = "ada"

has_console_handler = False


class Logger:
    """Logger class to be used by all modules in the project.

    The available logging levels are given below in decreasing order of severity:
        CRITICAL
        ERROR
        WARNING
        INFO
        DEBUG

    When a log_level is set to a logger, logging messages which are less severe than it will be ignored.
    For example, if the log_level is set to INFO, then DEBUG messages will be ignored.
    """

    def __init__(self, prefix=PREFIX, console: bool = True):
        self.root_logger = logging.getLogger(prefix)
        self.console = console
        self.log_level = "INFO"

    def get_root_logger(self, log_level="INFO", log_file=None):
        self._setup(log_level, log_file)
        return self.root_logger

    def set_log_level(self, log_level):
        self.log_level = log_level
        self.root_logger.setLevel(log_level)
        for handler in self.root_logger.handlers:
            handler.setLevel(log_level)

    def set_log_file(self, log_file):
        self._add_file_handler(log_file)

    def _setup(self, log_level, log_file):
        self.log_level = log_level
        self.root_logger.setLevel(log_level)
        if self.console:
            self._add_console_handler()
        coloredlogs.install(
            level=self.log_level,
            logger=self.root_logger,
            fmt="%(asctime)s %(hostname)s %(levelname)s %(message)s",
        )
        if log_file:
            self._add_file_handler(log_file)

    def _add_console_handler(self):
        # To avoid having duplicate logs in the console
        self.root_logger.propagate = False
        global has_console_handler
        if not has_console_handler:
            console_handler_formatter = logging.Formatter("%(filename)s [%(levelname)s]: %(message)s")
            console_handler = logging.StreamHandler()
            console_handler.setLevel(self.log_level)
            console_handler.setFormatter(console_handler_formatter)
            self.root_logger.addHandler(console_handler)
            has_console_handler = True

    def _add_file_handler(self, log_file):
        file_handler_formatter = logging.Formatter(
            "%(asctime)s %(filename)s [%(levelname)s]: %(message)s",
            datefmt="%Y-%m-%d %H:%M:%S",
        )
        file_handler = logging.FileHandler(log_file)
        file_handler.setLevel(self.log_level)
        file_handler.setFormatter(file_handler_formatter)
        self.root_logger.addHandler(file_handler)

    @staticmethod
    def get_logger(name):
        # We enforce the creation of a child logger (PREFIX.name) to keep the root logger setup
        if name.startswith(PREFIX + "."):
            return logging.getLogger(name)
        return logging.getLogger(PREFIX + "." + name)


class SshInteractiveChannel:
    def __init__(self, name: str, log: logging.Logger, channel: Channel):
        self.name = name
        # this get_pty trick ensures the command we execute will be terminated as soon as the ssh
        # channel is closed, which is what we want so we don't accumulate multiple run.sh processes on our
        # remote machines
        channel.get_pty()
        self.channel = channel
        self.stdout = channel.makefile()
        self.stderr = channel.makefile_stderr()
        self.log = log

    def start_listening(self):
        _thread.start_new_thread(self._log_lines, (self.stdout, self.channel.recv_ready, self.log.info))
        _thread.start_new_thread(
            self._log_lines,
            (self.stderr, self.channel.recv_stderr_ready, self.log.error),
        )

    def exec_command(self, cmd: str):
        self.log.info(f"### ssh {self.name} running {cmd} ...")
        bcmd = cmd.encode("utf-8")
        self.channel.exec_command(bcmd)

    def _log_lines(self, file: ChannelFile, readyfunc, logfunc):
        while not self.channel.closed:
            if readyfunc():
                data = file.readline()
                try:
                    line = data.strip()
                    logfunc(f"{self.name}: {line}")
                except UnicodeDecodeError:
                    pass
            else:
                time.sleep(1)

        if file == self.stdout:
            self.log.info(f"{self.name}: paramiko.Channel closed.")
            self.log.info(f"### ssh {self.name} command terminated")
