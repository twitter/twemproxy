
import os
import socket
import tempfile

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
nutcracker.
"""

def create_nutcracker_config_file():
    ncfg = tempfile.NamedTemporaryFile(prefix = "nutcracker-", suffix = ".yml")
    return ncfg

def enact_nutcracker_config(ncfg, nutcracker_config_yml):
    ncfg.seek(0)
    ncfg.truncate(0)
    ncfg.write(nutcracker_config_yml)
    ncfg.flush()

