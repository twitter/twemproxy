#!/usr/bin/env python

"""
This script tests that the client is not disconnected when the server restarts
during the request.
"""

import time
from test_helper import *


def test_server_fail_during_requests(cfg_yml_params):

    log("Testing server restart with parameters\n  %s" % cfg_yml_params)

    (listen_socket, server_port) = open_server_socket()

    log("Opened server on port %d" % server_port)

    port = suggest_spare_ports({'proxy':None, 'stats':None})

    # Create the nutcracker configuration out of the proxy and server ports.
    yml_cfg = simple_nutcracker_config(port['proxy'], server_port, cfg_yml_params)
    ncfg = create_nutcracker_config_file(yml_cfg)

    log("Opening nutcracker with config:\n%s" % yml_cfg);

    nut = NutcrackerProcess(["-c", ncfg.name,
                             "--stats-port", "%d" % port['stats']])

    # Send the request to the proxy. It is supposed to be captured by our
    # server socket, so check it immediately afterwards.

    client = tcp_connect(port['proxy'])
    client.send("get KEY\r\n")

    log("Accepting connection from proxy...")

    (server, _) = listen_socket.accept()
    should_receive(server, "get KEY \r\n")
    # DO NOT SEND A REPLY

    log("Closing server connection...")
    server.close()

    log("Awaiting SERVER_ERROR...")
    should_receive(client, "SERVER_ERROR unknown\r\n")

    log("Sending the second GET request...")
    # Do the request again. Should do just fine on a new connection.
    client.send("get KEY-after-close\r\n")

    log("Accepting a new connection from proxy...")
    (server, _) = listen_socket.accept()

    should_receive(server, "get KEY-after-close \r\n")

    server.close()
    listen_socket.close()

    print("Finished testing %s" % cfg_yml_params)


"""
Testing that server restart works across many parameters.
"""

variants_explored = []

for pc in [False, True]:
  cfg_yml_params = {'preconnect': pc,
      'server_retry_timeout': 42, # ms, Retry fast.
      # Disable auto_eject_hosts.
      # Otherwise we shall and will fail this test.
      'auto_eject_hosts': False,
      # (The server_failure_limit is only meaningful if aeh=True)
      }
  try:
      test_server_fail_during_requests(cfg_yml_params)
  except:
      log("Error while testing configuration: %s" % cfg_yml_params)
      raise
  variants_explored.append(cfg_yml_params)

log("%d nutcracker configuration variants successfully explored:\n%s"
        % (len(variants_explored), variants_explored))
