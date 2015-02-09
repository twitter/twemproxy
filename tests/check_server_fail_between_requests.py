#!/usr/bin/env python

"""
This script tests that the client is not disconnected when the server restarts
between the requests.
"""

import time
from test_helper import *


def test_server_fail_between_requests(test_settings, cfg_yml_params):

    log("Testing server restart with parameters\n  %s" % cfg_yml_params)

    (listen_socket, listen_port) = open_server_socket()

    log("Opened server on port %d" % listen_port)

    port = suggest_spare_ports({'proxy':None, 'stats':None})

    # Create the nutcracker configuration out of the proxy and server ports.
    yml_cfg = simple_nutcracker_config(port['proxy'], listen_port, cfg_yml_params)
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
    listen_socket.close();

    should_receive(server, "get KEY \r\n", log_suffix = "by the server")

    log("Server got to reply and close %s"
        % ["immediately", "after request is finished"][test_settings['die_right_after_END']])
    if test_settings['die_right_after_END'] == True:
        server.send("END\r\n")
        server.close()
        log("Oops, brought the server down right after replying!")
        should_receive(client, "END\r\n", log_suffix = "by the client")
    else:
        should_receive(client, "END\r\n", log_suffix = "by the client")
        log("Oops, time to bring the server down!")
        server.close()

    t = test_settings['time_to_keep_server_down']
    log("Simulate server restart delay (%g)..." % t)
    time.sleep(t)

    log("Server is ready to receive connections")
    (listen_socket, _) = open_server_socket(port = listen_port)

    t = test_settings['server_start_and_client_request_delta']
    time.sleep(t)

    # Do the request again. Should go through a different server connection.
    client.send("get KEY-after-close\r\n")

    log("Accepting a new connection from proxy...")
    (server, _) = listen_socket.accept()
    should_receive(server, "get KEY-after-close \r\n")
    server.send("END\r\n")
    should_receive(client, "END\r\n")

    server.close()

    print("Finished testing %s" % cfg_yml_params)


"""
Testing that server restart works across many parameters.
"""

variants_explored = []

for pc in [False, True]:
  for close_after_END in [False, True]:
    for server_keep_down in [0, 0.5]:
      for client_req_delta in [0, 0.1]:
            test_settings = {
                'die_right_after_END': close_after_END,
                'time_to_keep_server_down': server_keep_down,
                'server_start_and_client_request_delta': client_req_delta
                }
            cfg_yml_params = {'preconnect': pc,
                'server_retry_timeout': 42, # ms, Retry fast.
                # Disable auto_eject_hosts.
                # Otherwise we shall and will fail this test.
                'auto_eject_hosts': False,
                # (The server_failure_limit is only meaningful if aeh=True)
                }
            try:
                test_server_fail_between_requests(test_settings, cfg_yml_params)
            except:
                log("Error while testing configuration: %s\n  while %s"
                      % (cfg_yml_params, test_settings))
                raise
            variants_explored.append(cfg_yml_params)

log("%d nutcracker configuration variants successfully explored:\n%s"
        % (len(variants_explored), variants_explored))
