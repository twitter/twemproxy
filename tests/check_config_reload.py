#!/usr/bin/env python

import os
import time
import signal
import subprocess
from test_helper import *

try:
    nutcracker_exe = os.environ["NUTCRACKER_PROGRAM"]
except:
    print("This program is designed to run `make check` to test.")
    exit(1)


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

def simple_nutcracker_config(proxy_port, server_port):
    return ("alpha:\n"
            + "  listen: 127.0.0.1:%s\n" % proxy_port
            + "  hash: fnv1a_64\n"
            + "  distribution: ketama\n"
            + "  auto_eject_hosts: true\n"
            + "  server_retry_timeout: 2\n"
            + "  server_failure_limit: 1\n"
            #+ "  preconnect: true\n"
            + "  servers:\n"
            + "   - 127.0.0.1:%s:1 my_test_server\n" % server_port)

"""
Make a temporary file which will be used as a configuration for
nutcracker.
"""

def nutcracker_config_file():
    f = tempfile.NamedTemporaryFile(prefix = "nutcracker-", suffix = ".yml")
    return f


"""
Open two server side sockets.
One is is for configuration A, another is for configuration B.
We'll create a nutcracker configuration and make it reload from A to B.
"""

(listen_A, srv_A_port) = open_server_socket()
(listen_B, srv_B_port) = open_server_socket()

print("Opened two servers on ports %d and %d" % (srv_A_port, srv_B_port))


"""
Open the third server on a port which we'll later assign to be a proxy port
for nutcracker. This is a little bit dangerous due to potential race condition
with other servers on the local system, but we can't do much better than
that within the time and space constraints provided for testing. Just don't
run high load servers or clients on this development machine which does
integration testing.
"""

(proxy, proxy_port) = open_server_socket()
proxy.close()
(stats, stats_port) = open_server_socket()
stats.close()

"""
Create the nutcracker configurations out of the proxy and server ports.
"""

srv_A_cfg = simple_nutcracker_config(proxy_port, srv_A_port)
srv_B_cfg = simple_nutcracker_config(proxy_port, srv_B_port)

print("Prepared proxy port %d and stats port %d" % (proxy_port, stats_port))

ncfg = create_nutcracker_config_file()

print("Nutcracker config is in %s" % ncfg.name)

enact_nutcracker_config(ncfg, srv_A_cfg);

print("Opening nutcracker with config:\n%s" % srv_A_cfg);

nut_proc = subprocess.Popen([nutcracker_exe, "-c", ncfg.name,
                             "-a", "127.0.0.1", "-s", "%d" % stats_port])
time.sleep(0.1)
nut_proc.poll()
if nut_proc.returncode is not None:
    print("Could not start the nutcracker process: %s\n" % nut_proc.returncode);
    exit(1)


# Send the request to the proxy. It is supposed to be captured by our
# server socket, so check it immediately afterwards.

client = tcp_connect(proxy_port)
client.send("get KEY_FOR_A\r\n")

print "Accepting connection from proxy..."

(srv_A, _) = listen_A.accept()
data = srv_A.recv(128);
if data == "get KEY_FOR_A \r\n":
    print "Properly received get request by server A"
    srv_A.send("END\r\n")
else:
    print("Expectation failed: received data: \"%s\"" % data)
    exit(1)

print("Now, reloading the nutcracker with config:\n%s" % srv_B_cfg);

enact_nutcracker_config(ncfg, srv_B_cfg)
# Send the "config reload" signal.
nut_proc.send_signal(signal.SIGUSR1)
time.sleep(0.1)

client.send("get KEY_FOR_B\r\n")

(srv_B, _) = listen_B.accept()
data = srv_B.recv(128);
if data == "get KEY_FOR_B \r\n":
    print "Properly received get request by server B"
    srv_B.send("END\r\n")
else:
    print("Expectation failed: received data: \"%s\"" % data)
    exit(1)



nut_proc.kill()
nut_proc.wait()
if nut_proc.returncode is None:
    print("Could not stop the nutcracker process\n");
    exit(1)

print "Finished"

