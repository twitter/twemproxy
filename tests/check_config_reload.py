#!/usr/bin/env python

"""
This script tests that we continue to operate when the pool configuration
changes and reload is requested. Two properties are checked:
    1. That the reload effects the change indicated in the configuration,
       and starts going to a different server.
    2. That the client is not disconnected if the reload happens
       between the client requests.
"""

import time
from test_helper import *

def test_code_reload(cfg_yml_params):

    log("Testing code reload with parameters\n  %s" % cfg_yml_params)

    """
    Open two server side sockets.
    One is is for configuration A, another is for configuration B.
    We'll create a nutcracker configuration and make it reload from A to B.
    """

    (listen_A, srv_A_port) = open_server_socket()
    (listen_B, srv_B_port) = open_server_socket()

    log("Opened two servers on ports %d and %d" % (srv_A_port, srv_B_port))

    port = suggest_spare_ports({'proxy':None, 'stats':None})

    # Create the nutcracker configurations out of the proxy and server ports.

    srv_A_cfg = simple_nutcracker_config(port['proxy'], srv_A_port, cfg_yml_params)
    srv_B_cfg = simple_nutcracker_config(port['proxy'], srv_B_port, cfg_yml_params)

    ncfg = create_nutcracker_config_file()

    log("Nutcracker config is in %s" % ncfg.name)

    enact_nutcracker_config(ncfg, srv_A_cfg);

    log("Opening nutcracker with config:\n%s" % srv_A_cfg);

    nut = NutcrackerProcess(["-c", ncfg.name,
                             "--stats-port", "%d" % port['stats']])

    # Send the request to the proxy. It is supposed to be captured by our
    # server socket, so check it immediately afterwards.

    client = tcp_connect(port['proxy'])
    client.send("get KEY_FOR_A\r\n")

    log("Accepting connection from proxy...")

    (srv_A, _) = listen_A.accept()
    should_receive(srv_A, "get KEY_FOR_A \r\n")
    srv_A.send("END\r\n")

    log("Now, reloading the nutcracker with config:\n%s" % srv_B_cfg);

    enact_nutcracker_config(ncfg, srv_B_cfg)
    nut.config_reload()

    client.send("get KEY_FOR_B\r\n")

    (srv_B, _) = listen_B.accept()
    should_receive(srv_B, "get KEY_FOR_B \r\n")
    srv_B.send("END\r\n")

    srv_A.close()
    srv_B.close()

    listen_A.close()
    listen_B.close()

    print("Finished testing %s" % cfg_yml_params)


"""
Testing that reload works across many parameters.
"""

variants_explored = []

for pc in [False, True]:
  for aeh in [False, True]:
    for srt in [None, 2000]:
      for sfl in [1, 2]:
        cfg_yml_params = {'preconnect': pc,
                          'auto_eject_hosts': aeh,
                          'server_retry_timeout': srt,
                          'server_failure_limit': sfl}
        test_code_reload(cfg_yml_params)
        variants_explored.append(cfg_yml_params)

log("%d nutcracker configuration variants successfully explored:\n%s"
        % (len(variants_explored), variants_explored))