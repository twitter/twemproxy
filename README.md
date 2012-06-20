# twemproxy (nutcracker) [![Build Status](https://secure.travis-ci.org/twitter/twemproxy.png)](http://travis-ci.org/twitter/twemproxy)

**twemproxy** (pronounced "two-em-proxy"), aka **nutcracker** is a fast and lightweight proxy for memcached protocol. It was primarily built to reduce the connection count on the backend caching servers.

## Build ##

To build nutcracker from distribution tarball:

    $ ./configure
    $ make
    $ sudo make install

To build nutcracker from distribution tarball in _debug mode_:

    $ CFLAGS="-ggdb3 -O0" ./configure --enable-debug=full
    $ make
    $ sudo make install

To build nutcracker from source with _debug logs enabled_ and _assertions disabled_:

    $ git clone git@github.com:twitter/twemproxy.git
    $ cd twemproxy
    $ autoreconf -fvi
    $ ./configure --enable-debug=log
    $ make
    $ src/nutcracker -h

## Features ##

+ Fast.
+ Lightweight.
+ Maintains persistent server connections.
+ Keeps connection count on the backend caching servers low.
+ Enables pipelining of memcached requests and responses.
+ Supports proxying to multiple servers.
+ Supports multiple server pools simultaneously.
+ Implements the complete memcached ASCII protocol.
+ Easy configuration of server pools through a YAML file.
+ Supports multiple hashing modes including consistent hashing and distribution.
+ Observability through stats exposed on stats monitoring port.

## Help ##

    Usage: nutcracker [-?hVdt] [-v verbosity level] [-o output file]
                      [-c conf file] [-s stats port] [-i stats interval]
                      [-p pid file] [-m mbuf size]

    Options:
      -h, --help             : this help
      -V, --version          : show version and exit
      -t, --test-conf        : test configuration for syntax errors and exit
      -d, --daemonize        : run as a daemon
      -v, --verbosity=N      : set logging level (default: 5, min: 0, max: 11)
      -o, --output=S         : set logging file (default: stderr)
      -c, --conf-file=S      : set configuration file (default: conf/nutcracker.yml)
      -s, --stats-port=N     : set stats monitoring port (default: 22222)
      -i, --stats-interval=N : set stats aggregation interval in msec (default: 30000 msec)
      -p, --pid-file=S       : set pid file (default: off)
      -m, --mbuf-size=N      : set size of mbuf chunk in bytes (default: 16384 bytes)

## Configuration ##

nutcracker can be configured through a YAML file specified by the -c or --conf-file command-line argument on process start. The configuration file is used to specify the server pools and the servers within each pool that nutcracker manages. The configuration files parses and understands the following keys:

+ **listen**: The listening address and port (name:port or ip:port) for this server pool.
+ **hash**: The name of the hash function. Possible values are: one_at_a_time, md5, crc32, fnv1_64, fnv1a_64, fnv1_32, fnv1a_32, hsieh, murmur, and jenkins.
+ **distribution**: The key distribution mode. Possible values are: ketama, modula and random.
+ **timeout**: The timeout value in msec that we wait for to establish a connection to the server or receive a response from a server. By default, we wait indefinitely.
+ **backlog**: The TCP backlog argument. Defaults to 512.
+ **preconnect**: A boolean value that controls if nutcracker should preconnect to all the servers in this pool on process start. Defaults to false.
+ **server_connections**: The maximum number of connections that can be opened to each server. By default, we open at most 1 server connection.
+ **auto_eject_hosts**: A boolean value that controls if server should be ejected temporarily when it fails consecutively server_failure_limit times. Defaults to false.
+ **server_retry_timeout**: The timeout value in msec to wait for before retrying on a temporarily ejected server, when auto_eject_host is set to true. Defaults to 30000 msec.
+ **server_failure_limit**: The number of conseutive failures on a server that would leads to it being temporarily ejected when auto_eject_host is set to true. Defaults to 2.
+ **servers**: A list of server address, port and weight (name:port:weight or ip:port:weight) for this server pool.

For example, the configuration file in conf/nutcracker.yml, also shown below, configures 4 server pools with names -  _alpha_, _beta_, _gamma_ and _delta_. Clients that intend to send requests to one of the 10 servers in pool gamma connect to port 22123 on 127.0.0.1. Clients that intend to send request to one of 2 servers in pool delta connect to unix path /tmp/gamma. Requests sent to pool alpha and delta have no timeout and might require timeout functionality to be implemented on the client side. On the other hand, requests sent to pool beta and gamma timeout after 400 msec and 100 msec respectively when no response is received from the server. Of the 4 server pools, only pools alpha, beta and gamma are configured to use server ejection and hence are resilient to server failures. All the 4 server pools use ketama consistent hashing for key distribution with the key hasher for pools alpha, beta and gamma set to fnv1a_64 while that for pool delta set to hsieh.

    alpha:
      listen: 127.0.0.1:22121
      hash: fnv1a_64
      distribution: ketama
      auto_eject_hosts: true
      server_retry_timeout: 2000
      server_failure_limit: 1
      servers:
       - 127.0.0.1:11211:1

    beta:
      listen: 127.0.0.1:22122
      hash: fnv1a_64
      distribution: ketama
      timeout: 400
      backlog: 1024
      preconnect: true
      auto_eject_hosts: true
      server_retry_timeout: 2000
      server_failure_limit: 3
      servers:
       - 127.0.0.1:11212:1
       - 127.0.0.1:11213:1

    gamma:
      listen: 127.0.0.1:22123
      hash: fnv1a_64
      distribution: ketama
      timeout: 100
      auto_eject_hosts: true
      server_retry_timeout: 2000
      server_failure_limit: 1
      servers:
       - 127.0.0.1:11214:1
       - 127.0.0.1:11215:1
       - 127.0.0.1:11216:1
       - 127.0.0.1:11217:1
       - 127.0.0.1:11218:1
       - 127.0.0.1:11219:1
       - 127.0.0.1:11220:1
       - 127.0.0.1:11221:1
       - 127.0.0.1:11222:1
       - 127.0.0.1:11223:1

    delta:
      listen: /tmp/gamma
      hash: hsieh
      distribution: ketama
      auto_eject_hosts: false
      servers:
       - 127.0.0.1:11214:100000
       - 127.0.0.1:11215:1

Finally, to make writing syntactically correct configuration file easier, nutcracker provides a command-line argument -t or --test-conf that can be used to test the YAML configuration file for any syntax error.

## Observability ##

Observability in nutcracker is through logs and stats.

Nutcracker exposes stats at the granularity of server pool and servers per pool through the stats monitoring port. The stats are essentially JSON formatted key-value pairs, with the keys corresponding to counter names. By default stats are exposed on port 22222 and aggregated every 30 seconds. Both these values can be configured on program start using the -c or --conf-file and -i or --stats-interval command-line arguments respectively.

Logging in nutcracker is only available when nutcracker is built with logging enabled. By default logs are written to stderr. Nutcracker can also be configured to write logs to a specific file through the -o or --output command-line argument. On a running nutcracker, we can turn log levels up and down by sending it SIGTTIN and SIGTTOU signals respectively and reopen log files by sending it SIGHUP signal.

## Issues and Support ##

Have a bug or a question? Please create an issue here on GitHub!

https://github.com/twitter/twemproxy/issues

## Contributors ##

* Manju Rajashekhar (@manju)

## License ##

Copyright 2012 Twitter, Inc.

Licensed under the Apache License, Version 2.0: http://www.apache.org/licenses/LICENSE-2.0
