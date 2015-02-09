# -*- coding: utf-8 -*-

import os
import sys
import time
import fcntl
import signal
import socket
import struct
import fnmatch
import tempfile
import datetime
import subprocess

def log(str):
    ts = datetime.datetime.now()
    sys.stderr.write("[%s]: 💥  %s\n" % (ts, str))
    sys.stderr.flush()

"""
Wrap the socket so it does not fail on BSD system because of EAGAIN
"""
class Sock(socket.socket):
    def __init__(self, _sock = None):
        socket.socket.__init__(self, socket.AF_INET, socket.SOCK_STREAM, _sock = _sock);

        self.setblocking(True)
        self.settimeout(3.0)

        # Descriptor must not leak into nutcracker process.
        flags = fcntl.fcntl(self.fileno(), fcntl.F_GETFD)
        fcntl.fcntl(self.fileno(), fcntl.F_SETFD, flags | fcntl.FD_CLOEXEC)

        self.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        # All closes result in RST, not FIN. Simulate hard server failure.
        l_onoff = 1
        l_linger = 0
        self.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER,
                        struct.pack('ii', l_onoff, l_linger))

    def accept(self):
        (s, addr) = socket.socket.accept(self)
        return (Sock(_sock = s), addr)


"""
Teach python to open a socket on some first available local port.
Return the socket object and the local port obtained.
"""
def open_server_socket(port = 0):
    s = Sock()
    s.bind(("127.0.0.1", port))
    s.listen(1)
    (_, port) = s.getsockname()
    return (s, port)


"""
Open a client socket and connect to the given port on local machine.
"""

def tcp_connect(port):
    s = Sock()
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


# Make a temporary file which will be used as a configuration for
# nutcracker. Fill it up with a specified configuration, if given.
def create_nutcracker_config_file(config_yml = None):
    ncfg = tempfile.NamedTemporaryFile(prefix = "nutcracker-", suffix = ".yml")
    if config_yml is not None:
        enact_nutcracker_config(ncfg, config_yml)
    return ncfg


# Save the new nutcracker configuration to the file system.
def enact_nutcracker_config(ncfg, nutcracker_config_yml):
    ncfg.seek(0)
    ncfg.truncate(0)
    ncfg.write(nutcracker_config_yml)
    ncfg.flush()


# Try to receive data from the socket and match against the given value.
def should_receive(conn, value_pattern, log_suffix = ""):
    data = conn.recv(128);
    if fnmatch.fnmatch(data, value_pattern):
        log("Properly received %r %s" % (data, log_suffix))
    else:
        log("Expectation failed: received data: %r %s" % (data, log_suffix))
        raise


# Open the servers on ports which we'll later assign to be a proxy port
# and a stats port for nutcracker. This is a little bit dangerous due to
# potential race conditions with the other servers on the local system,
# but we can't do much better than that within the time and space constraints
# provided for testing.
# Just don't run high load servers or clients on this development machine
# which does integration testing.
def suggest_spare_ports(dict):
    for key in dict:
        (s, port) = open_server_socket()
        s.close()
        dict[key] = port
    log("Prepared extra ports %r" % dict)
    return dict


# A resource wrapper for the nutcracker process.
class NutcrackerProcess(object):
    def __init__(self, args):
        self.proc = None

        try:
            exe = os.environ["NUTCRACKER_PROGRAM"]
        except:
            print("This program is designed to run under `make check`.")
            print("Set NUTCRACKER_PROGRAM environment variable and try again.")
            exit(1)

        self.proc = subprocess.Popen([exe, "--stats-addr", "127.0.0.1", "-v8"]
                                     + args)
        time.sleep(0.1)
        self.proc.poll()
        if self.proc.returncode is not None:
            log("Could not start the nutcracker process: %r\n"
                    % nut_proc.returncode);
            raise
        log("Started nutcracker pid %r" % self.proc.pid);

    def config_reload(self):
        self.proc.send_signal(signal.SIGUSR1)
        time.sleep(0.1)

    def __del__(self):
        if self.proc:
            self.proc.kill()
            self.proc.wait()
            if self.proc.returncode is None:
                log("Could not stop the nutcracker process\n");
                raise
            log("Stopped nutcracker pid %r" % self.proc.pid);

