#!/usr/bin/env python

"""
Test that we can switch to a new configuration after the old configuration
got totally hosed with all the servers deemed broken.
"""

from test_helper import *

def test_code_reload(test_settings, cfg_yml_params):

    log("Testing code reload with parameters\n  %s\n  while %s" % (cfg_yml_params, test_settings))

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
    listen_A.close()    # Accept on A only once.
    should_receive(srv_A, "get KEY_FOR_A \r\n")

    if test_settings['fail_during_request']:
      srv_A.close();
      should_receive(client, "SERVER_ERROR *", log_suffix = "by the client")
      client.close();
    else:
      srv_A.send("END\r\n")
      srv_A.close()
      should_receive(client, "END\r\n", log_suffix = "by the client")
      client.close()
      time.sleep(0.01)  # Let the logging flush in nutcracker, courtesy.

    log("Current env is %s %s port %d" % (cfg_yml_params, test_settings, port['proxy']))
    log("Connecting the second time and check that it does not work")

    client = tcp_connect(port['proxy'])
    client.settimeout(50)
    client.send("get KEY_FOR_FAILED_A\r\n")
    should_receive(client, "SERVER_ERROR *", log_suffix = "by client (2)")
    should_receive(client, "", log_suffix = "by client (2)")
    client.close()

    time.sleep(0.1)

    log("Connecting the third time, config switching to B, should work now")

    client = tcp_connect(port['proxy'])

    log("Switching nutcracker to server B")
    enact_nutcracker_config(ncfg, srv_B_cfg)
    nut.config_reload()

    log("The subsequent request should go through the server B")
    client.send("get KEY_FOR_B\r\n")
    log("Server B is now awaiting nutcracker on port %d" % srv_B_port)
    (srv_B, _) = listen_B.accept()
    should_receive(srv_B, "get KEY_FOR_B \r\n", log_suffix = "by server B")
    srv_B.send("END\r\n")
    should_receive(client, "END\r\n", log_suffix = "by client (3)")

    client.close()
    srv_B.close()

    listen_B.close()

    log("Finished testing %s" % cfg_yml_params)


"""
Testing that reload works across many parameters.
"""

variants_explored = []

for pc in [False, True]:
  for aeh in [False, True]:
    for srt in [200, 1000]:
      for sfl in [1]:
        for during_req in [False, True]:
          test_settings = {'fail_during_request': during_req }
          cfg_yml_params = {'preconnect': pc,
                            'auto_eject_hosts': aeh,
                            'server_retry_timeout': srt,
                            'server_failure_limit': sfl}
          try:
            test_code_reload(test_settings, cfg_yml_params)
            variants_explored.append(cfg_yml_params)
          except:
            log("Error while testing configuration: %s\n  while %s"
                % (cfg_yml_params, test_settings))
            raise

log("%d nutcracker configuration variants successfully explored:\n%s"
        % (len(variants_explored), variants_explored))
