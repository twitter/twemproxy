#!/usr/bin/env python

"""
This script tests that the client is not disconnected when the server restarts
during the request or between the requests.
"""

import time
from test_helper import *

def should_receive(conn, value):
    data = conn.recv(128);
    if data == value:
        print("Properly received %r" % data)
    else:
        print("Expectation failed: received data: %r" % data)
        exit(1)

def test_server_restart(cfg_yml_params, fail_during_request):

    print "Testing server restart with parameters\n  %s" % cfg_yml_params

    (listen_socket, server_port) = open_server_socket()

    print("Opened server on port %d" % server_port)

    """
    Open the server on a port which we'll later assign to be a proxy port
    for nutcracker. This is a little bit dangerous due to potential race
    condition with other servers on the local system, but we can't do much
    better than that within the time and space constraints provided for testing.
    Just don't run high load servers or clients on this development machine
    which does integration testing.
    """

    (proxy, proxy_port) = open_server_socket()
    proxy.close()
    (stats, stats_port) = open_server_socket()
    stats.close()

    print("Prepared proxy port %d and stats port %d" % (proxy_port, stats_port))

    """
    Create the nutcracker configuration out of the proxy and server ports.
    """

    yml_cfg = simple_nutcracker_config(proxy_port, server_port, cfg_yml_params)

    ncfg = create_nutcracker_config_file(yml_cfg)

    print("Opening nutcracker with config:\n%s" % yml_cfg);

    nut = NutcrackerProcess(["-c", ncfg.name,
                             "--stats-port", "%d" % stats_port])

    # Send the request to the proxy. It is supposed to be captured by our
    # server socket, so check it immediately afterwards.

    client = tcp_connect(proxy_port)
    client.send("get KEY\r\n")

    print "Accepting connection from proxy..."

    (server, _) = listen_socket.accept()
    should_receive(server, "get KEY \r\n")

    if fail_during_request:
        server.close()
        should_receive(client, "SERVER_ERROR unknown\r\n")

        print "Accepting a new connection from proxy..."
        (server, _) = listen_socket.accept()

        # Do the request again. Should do just fine on a new connection.
        client.send("get KEY-after-close\r\n")
        should_receive(server, "get KEY-after-close \r\n")
    else:   # Fail between requests.
        server.send("END\r\n")
        # Close, open, then try to send the request again. Should do just fine.
        time.sleep(0.1)
        server.close()
        time.sleep(0.1)
        client.send("get KEY-after-close\r\n")
        (server, _) = listen_socket.accept()
        should_receive(server, "get KEY-after-close \r\n")

    server.close()
    listen_socket.close()

    print "Finished testing %s" % cfg_yml_params


"""
Testing that server restart works across many parameters.
"""

variants_explored = []

for fail_during_request in [True, False]:   # Fail during or between requests
  for pc in [False, True]:
    cfg_yml_params = {'preconnect': pc,
        'server_retry_timeout': 42, # ms, Retry fast.
        # Disable auto_eject_hosts.
        # Otherwise we shall and will fail this test.
        'auto_eject_hosts': False,
        # (The server_failure_limit is only meaningful if aeh=True)
        }
    try:
        test_server_restart(cfg_yml_params, fail_during_request)
    except:
        print("Error while testing configuration: %s (%s requests)"
            % (cfg_yml_params, ("between", "during")[fail_during_request]))
        raise
    variants_explored.append(cfg_yml_params)

print("%d nutcracker configuration variants successfully explored:\n%s"
        % (len(variants_explored), variants_explored))
