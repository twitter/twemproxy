#!/usr/bin/env python

"""
This script tests that the client is not disconnected when the server restarts
between the requests.
"""

import time
from test_helper import *


def test_server_fail_between_requests(cfg_yml_params):

    print("Testing server restart with parameters\n  %s" % cfg_yml_params)

    (listen_socket, server_port) = open_server_socket()

    print("Opened server on port %d" % server_port)

    port = suggest_spare_ports({'proxy':None, 'stats':None})

    # Create the nutcracker configuration out of the proxy and server ports.
    yml_cfg = simple_nutcracker_config(port['proxy'], server_port, cfg_yml_params)
    ncfg = create_nutcracker_config_file(yml_cfg)

    print("Opening nutcracker with config:\n%s" % yml_cfg);

    nut = NutcrackerProcess(["-c", ncfg.name,
                             "--stats-port", "%d" % port['stats']])

    # Send the request to the proxy. It is supposed to be captured by our
    # server socket, so check it immediately afterwards.

    client = tcp_connect(port['proxy'])
    client.send("get KEY\r\n")

    print("Accepting connection from proxy...")

    (server, _) = listen_socket.accept()
    should_receive(server, "get KEY \r\n")
    server.send("END\r\n")

    # Close, open, then try to send the request again. Should do just fine.
    server.close()
    should_receive(client, "END\r\n")

    # Do the request again. Should do just fine on a new connection.
    client.send("get KEY-after-close\r\n")

    print("Accepting a new connection from proxy...")
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
      test_server_fail_between_requests(cfg_yml_params)
  except:
      print("Error while testing configuration: %s" % cfg_yml_params)
      raise
  variants_explored.append(cfg_yml_params)

print("%d nutcracker configuration variants successfully explored:\n%s"
        % (len(variants_explored), variants_explored))
