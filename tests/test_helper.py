
import os
import time
import signal
import socket
import tempfile
import subprocess

"""
Teach python to open a socket on some first available local port.
Return the socket object and the local port obtained.
"""

def open_server_socket():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind(("127.0.0.1", 0))
    s.listen(1)
    s.settimeout(3.0)
    (_, port) = s.getsockname()
    return (s, port)


"""
Open a client socket and connect to the given port on local machine.
"""

def tcp_connect(port):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("127.0.0.1", port))
    return s


"""
Create a simple yaml config with a given port as a server.
"""

def simple_nutcracker_config(proxy_port, server_port, dict):
    auto_eject_hosts = dict.get("auto_eject_hosts", False)
    server_retry_timeout = dict.get("server_retry_timeout", 2000)
    server_failure_limit = dict.get("server_failure_limit", 1)
    preconnect = dict.get("preconnect", True)

    return ("alpha:\n"
            + "  listen: 127.0.0.1:%s\n" % proxy_port
            + "  hash: fnv1a_64\n"
            + "  distribution: ketama\n"
            + "  auto_eject_hosts: %s\n" % ("true" if auto_eject_hosts else "false")
            + ("" if server_retry_timeout is None else
              "  server_retry_timeout: %s\n" % server_retry_timeout)
            + "  server_failure_limit: %d\n" % server_failure_limit
            + "  preconnect: %s\n" % ("true" if preconnect else "false")
            + "  servers:\n"
            + "   - 127.0.0.1:%s:1 my_test_server\n" % server_port)


"""
Make a temporary file which will be used as a configuration for
nutcracker. Fill it up with a specified configuration, if given.
"""

def create_nutcracker_config_file(config_yml = None):
    ncfg = tempfile.NamedTemporaryFile(prefix = "nutcracker-", suffix = ".yml")
    if config_yml is not None:
        enact_nutcracker_config(ncfg, config_yml)
    return ncfg

def enact_nutcracker_config(ncfg, nutcracker_config_yml):
    ncfg.seek(0)
    ncfg.truncate(0)
    ncfg.write(nutcracker_config_yml)
    ncfg.flush()


class NutcrackerProcess(object):
    def __init__(self, args):
        self.proc = subprocess.Popen(args)
        time.sleep(0.1)
        self.proc.poll()
        if self.proc.returncode is not None:
            print("Could not start the nutcracker process: %r\n"
                    % nut_proc.returncode);
            raise
        print("Started nutcracker pid %r" % self.proc.pid);

    def config_reload(self):
        self.proc.send_signal(signal.SIGUSR1)
        time.sleep(0.1)

    def __del__(self):
        self.proc.kill()
        self.proc.wait()
        if self.proc.returncode is None:
            print("Could not stop the nutcracker process\n");
            raise
        print("Stopped nutcracker pid %r" % self.proc.pid);

