#!/usr/bin/env python

"""
The script checks what happens with the clients which came after we detected
the server failure.
"""

from test_helper import *

def test_server_fail_and_recover(test_settings, cfg_yml_params):

    log("Testing code reload with parameters\n  %s\n  while %s" % (cfg_yml_params, test_settings))

    (listen, server_port) = open_server_socket()

    log("Opened a server on port %d" % server_port)

    port = suggest_spare_ports({'proxy':None, 'stats':None})

    # Create the nutcracker configurations out of the proxy and server ports.

    server_cfg = simple_nutcracker_config(port['proxy'], server_port, cfg_yml_params)

    ncfg = create_nutcracker_config_file()

    log("Nutcracker config is in %s" % ncfg.name)

    enact_nutcracker_config(ncfg, server_cfg);

    log("Opening nutcracker with config:\n%s" % server_cfg);

    nut = NutcrackerProcess(["-c", ncfg.name,
                             "--stats-port", "%d" % port['stats']])

    # Send the request to the proxy. It is supposed to be captured by our
    # server socket, so check it immediately afterwards.

    client = tcp_connect(port['proxy'])
    client.send("get KEY\r\n")

    log("Accepting connection from proxy...")

    (server, _) = listen.accept()
    listen.close()    # Accept on server only once.
    should_receive(server, "get KEY \r\n")

    if test_settings['fail_during_request']:
      server.close();
      should_receive(client, "SERVER_ERROR *", log_suffix = "by the client")
      client.close();
    else:
      server.send("END\r\n")
      server.close()
      should_receive(client, "END\r\n", log_suffix = "by the client")
      client.close()
      time.sleep(0.01)  # Let the logging flush in nutcracker, courtesy.


    log("Connecting the second time and check that it does not work")

    client = tcp_connect(port['proxy'])
    client.send("get KEY\r\n")
    should_receive(client, "SERVER_ERROR *", log_suffix = "by the client (2)")
    should_receive(client, "", log_suffix = "by the client (2)")
    client.close();

    time.sleep(0.1)

    log("Connecting the third time and check that it works")

    client = tcp_connect(port['proxy'])
    (listen, server_port) = open_server_socket(port = server_port)

    log("The subsequent request should eventually timeout")
    client.send("get KEY\r\n")
    log("Server is now awaiting nutcracker on port %d" % server_port)
    (server, _) = listen.accept()
    should_receive(server, "get KEY \r\n", log_suffix = "by server")
    server.send("END\r\n")
    should_receive(client, "END\r\n", log_suffix = "by client (3)")

    server.close()
    listen.close()

    log("Finished testing %s" % cfg_yml_params)


"""
Testing that reload works across many parameters.
"""

variants_explored = []

for pc in [False, True]:
  # Fix auto_eject_hosts to False, since ejected hosts aren't going to
  # recover after failure anyway.
  for aeh in [False]:
    for srt in [200, 1000]:
      for sfl in [1]:
        for during_req in [False, True]:
          test_settings = {'fail_during_request': during_req }
          cfg_yml_params = {'preconnect': pc,
                            'auto_eject_hosts': aeh,
                            'server_retry_timeout': srt,
                            'server_failure_limit': sfl}
          try:
            test_server_fail_and_recover(test_settings, cfg_yml_params)
            variants_explored.append(cfg_yml_params)
          except:
            log("Error while testing configuration: %s\n  while %s"
                % (cfg_yml_params, test_settings))
            raise

log("%d nutcracker configuration variants successfully explored:\n%s"
        % (len(variants_explored), variants_explored))
