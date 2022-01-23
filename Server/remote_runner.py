# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
import io
import sys
from threading import Thread
import paramiko


class RemoteRunner:
    def __init__(self, ipaddress=None, username=None, password=None,
                 command=None, verbose=True, logfile=None,
                 timeout=None, ):

        self.ipaddress = ipaddress
        self.username = username
        self.password = password
        self.command = command
        self.verbose = verbose
        self.timeout = timeout
        if logfile:
            self.logfile = open(logfile, 'w')
        else:
            self.logfile = None
        self.all = all
        self.machine = None
        self.ssh = None
        self.buffer = None

    def connect_ssh(self):
        self.ssh = paramiko.SSHClient()
        self.ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        self.print("connecting to target device at " + self.ipaddress)
        self.ssh.connect(self.ipaddress, username=self.username, password=self.password)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, exc_traceback):
        if self.ssh:
            self.ssh.close()

    def close_ssh(self):
        self.ssh.close()

    def logstream(self, stream):
        try:
            while True:
                out = stream.readline()
                if out:
                    msg = out.rstrip('\n')
                    self.print(msg)
                else:
                    break
        except:
            errorType, value, traceback = sys.exc_info()
            msg = "### logstream exception: %s: %s" % (str(errorType), str(value))
            self.print(msg)

    def exec_remote_command(self, cmd):
        self.print("remote: " + cmd)
        self.buffer = io.StringIO()
        try:
            stdin, stdout, stderr = self.ssh.exec_command(cmd, timeout=self.timeout)

            stdout_thread = Thread(target=self.logstream, args=(stdout,))
            stderr_thread = Thread(target=self.logstream, args=(stderr,))

            stdout_thread.start()
            stderr_thread.start()

            while stdout_thread.isAlive() or stderr_thread.isAlive():
                pass

        except:
            errorType, value, traceback = sys.exc_info()
            msg = "### exec_remote_command exception: %s: %s" % (str(errorType), str(value))
            self.print(msg)

        result = self.buffer.getvalue().split('\n')
        self.buffer = None
        return result

    def print(self, output):
        if self.verbose:
            print(output)
        if self.buffer:
            self.buffer.write(output + "\n")
        if self.logfile:
            self.logfile.write(output + "\n")

    def run_command(self):
        output = []
        try:
            self.connect_ssh()
            if self.start_clean:
                self.clean_target()
            self.publish_bits()
            if self.command:
                if self.target_dir:
                    self.exec_remote_command("cd {} && chmod u+x ./{}".format(
                        self.target_dir, self.command.split(" ")[0]))

                    output = self.exec_remote_command("cd {} && ./{}".format(
                        self.target_dir, self.command))
                else:
                    output = self.exec_remote_command(self.command)
            self.copy_files()
            if self.cleanup:
                self.clean_target()
            self.close_ssh()
        except:
            errorType, value, traceback = sys.exc_info()
            msg = "### run_command exception: %s: %s" % (str(errorType), str(value) + "\n" + str(traceback))
            self.print(msg)
            if self.buffer:
                output = self.buffer.getvalue().split('\n')
            output += [msg]
        return output

    def run_all(self):
        for machine in self.cluster.get_all():
            try:
                self.ipaddress = machine.ip_address
                self.run_command()
            except:
                errorType, value, traceback = sys.exc_info()
                self.print("### Unexpected Exception: " + str(errorType) + ": " + str(value) + "\n" + str(traceback))


if __name__ == "__main__":

    logging.basicConfig(level=logging.INFO, format="%(message)s")

    import argparse
    arg_parser = argparse.ArgumentParser("remoterunner executes remote commands on a given machine")

    arg_parser.add_argument("--ipaddress", help="Address of machine to run commands on", required=True)
    arg_parser.add_argument("--username", help="Username for logon to remote machine", default=None)
    arg_parser.add_argument("--password", help="Password for logon to remote machine", default=None)
    arg_parser.add_argument("--command", help="The command to run on the remote machine", default=None)
    arg_parser.add_argument("--logfile", help="The name of logfile to write to", default=None)
    arg_parser.add_argument("--timeout", type=bool, help="Timeout for the command in seconds (default 300 seconds)",
                            default=300)

    args = arg_parser.parse_args()

    with RemoteRunner(ipaddress=args.ipaddress, username=args.username, password=args.password,
                      command=args.command, verbose=True, logfile=args.logfile, timeout=args.timeout) as runner:
        runner.run_command()
