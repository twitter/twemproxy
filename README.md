# twemproxy (nutcracker) [![Build Status](https://secure.travis-ci.org/twitter/twemproxy.png)](http://travis-ci.org/twitter/twemproxy)

**twemproxy** (pronounced "two-em-proxy"), aka **nutcracker** is a fast and lightweight proxy for [memcached](http://www.memcached.org/) and [redis](http://redis.io/) protocol. It was built primarily to reduce the number of connections to the caching servers on the backend. This, together with protocol pipelining and sharding enables you to horizontally scale your distributed caching architecture.

## Build

To build twemproxy from [distribution tarball](https://drive.google.com/open?id=0B6pVMMV5F5dfMUdJV25abllhUWM&authuser=0):

    $ ./configure
    $ make
    $ sudo make install

To build twemproxy from [distribution tarball](https://drive.google.com/open?id=0B6pVMMV5F5dfMUdJV25abllhUWM&authuser=0) in _debug mode_:

    $ CFLAGS="-ggdb3 -O0" ./configure --enable-debug=full
    $ make
    $ sudo make install

To build twemproxy from source with _debug logs enabled_ and _assertions enabled_:

    $ git clone git@github.com:twitter/twemproxy.git
    $ cd twemproxy
    $ autoreconf -fvi
    $ ./configure --enable-debug=full
    $ make
    $ src/nutcracker -h

A quick checklist:

+ Use newer version of gcc (older version of gcc has problems)
+ Use CFLAGS="-O1" ./configure && make
+ Use CFLAGS="-O3 -fno-strict-aliasing" ./configure && make
+ `autoreconf -fvi && ./configure` needs `automake` and `libtool` to be installed

## Features

+ Fast.
+ Lightweight.
+ Maintains persistent server connections.
+ Keeps connection count on the backend caching servers low.
+ Enables pipelining of requests and responses.
+ Supports proxying to multiple servers.
+ Supports multiple server pools simultaneously.
+ Shard data automatically across multiple servers.
+ Implements the complete [memcached ascii](notes/memcache.md) and [redis](notes/redis.md) protocol.
+ Easy configuration of server pools through a YAML file.
+ Supports multiple hashing modes including consistent hashing and distribution.
+ Can be configured to disable nodes on failures.
+ Observability via stats exposed on the stats monitoring port.
+ Works with Linux, *BSD, OS X and SmartOS (Solaris)

## Help

    Usage: nutcracker [-?hVdDt] [-v verbosity level] [-o output file]
                      [-c conf file] [-s stats port] [-a stats addr]
                      [-i stats interval] [-p pid file] [-m mbuf size]

    Options:
      -h, --help             : this help
      -V, --version          : show version and exit
      -t, --test-conf        : test configuration for syntax errors and exit
      -d, --daemonize        : run as a daemon
      -D, --describe-stats   : print stats description and exit
      -v, --verbose=N        : set logging level (default: 5, min: 0, max: 11)
      -o, --output=S         : set logging file (default: stderr)
      -c, --conf-file=S      : set configuration file (default: conf/nutcracker.yml)
      -s, --stats-port=N     : set stats monitoring port (default: 22222)
      -a, --stats-addr=S     : set stats monitoring ip (default: 0.0.0.0)
      -i, --stats-interval=N : set stats aggregation interval in msec (default: 30000 msec)
      -p, --pid-file=S       : set pid file (default: off)
      -m, --mbuf-size=N      : set size of mbuf chunk in bytes (default: 16384 bytes)

## Zero Copy

In twemproxy, all the memory for incoming requests and outgoing responses is allocated in mbuf. Mbuf enables zero-copy because the same buffer on which a request was received from the client is used for forwarding it to the server. Similarly the same mbuf on which a response was received from the server is used for forwarding it to the client.

Furthermore, memory for mbufs is managed using a reuse pool. This means that once mbuf is allocated, it is not deallocated, but just put back into the reuse pool. By default each mbuf chunk is set to 16K bytes in size. There is a trade-off between the mbuf size and number of concurrent connections twemproxy can support. A large mbuf size reduces the number of read syscalls made by twemproxy when reading requests or responses. However, with a large mbuf size, every active connection would use up 16K bytes of buffer which might be an issue when twemproxy is handling large number of concurrent connections from clients. When twemproxy is meant to handle a large number of concurrent client connections, you should set chunk size to a small value like 512 bytes using the -m or --mbuf-size=N argument.

## Configuration

Twemproxy can be configured through a YAML file specified by the -c or --conf-file command-line argument on process start. The configuration file is used to specify the server pools and the servers within each pool that twemproxy manages. The configuration files parses and understands the following keys:

+ **listen**: The listening address and port (name:port or ip:port) or an absolute path to sock file (e.g. /var/run/nutcracker.sock) for this server pool.
+ **client_connections**: The maximum number of connections allowed from redis clients. Unlimited by default, though OS-imposed limitations will still apply.
+ **hash**: The name of the hash function. Possible values are:
 + one_at_a_time
 + md5
 + crc16
 + crc32 (crc32 implementation compatible with [libmemcached](http://libmemcached.org/))
 + crc32a (correct crc32 implementation as per the spec)
 + fnv1_64
 + fnv1a_64
 + fnv1_32
 + fnv1a_32
 + hsieh
 + murmur
 + jenkins
+ **hash_tag**: A two character string that specifies the part of the key used for hashing. Eg "{}" or "$$". [Hash tag](notes/recommendation.md#hash-tags) enable mapping different keys to the same server as long as the part of the key within the tag is the same.
+ **distribution**: The key distribution mode. Possible values are:
 + ketama
 + modula
 + random
+ **timeout**: The timeout value in msec that we wait for to establish a connection to the server or receive a response from a server. By default, we wait indefinitely.
+ **backlog**: The TCP backlog argument. Defaults to 512.
+ **preconnect**: A boolean value that controls if twemproxy should preconnect to all the servers in this pool on process start. Defaults to false.
+ **redis**: A boolean value that controls if a server pool speaks redis or memcached protocol. Defaults to false.
+ **redis_auth**: Authenticate to the Redis server on connect.
+ **redis_db**: The DB number to use on the pool servers. Defaults to 0. Note: Twemproxy will always present itself to clients as DB 0.
+ **server_connections**: The maximum number of connections that can be opened to each server. By default, we open at most 1 server connection.
+ **auto_eject_hosts**: A boolean value that controls if server should be ejected temporarily when it fails consecutively server_failure_limit times. See [liveness recommendations](notes/recommendation.md#liveness) for information. Defaults to false.
+ **server_retry_timeout**: The timeout value in msec to wait for before retrying on a temporarily ejected server, when auto_eject_host is set to true. Defaults to 30000 msec.
+ **server_failure_limit**: The number of consecutive failures on a server that would lead to it being temporarily ejected when auto_eject_host is set to true. Defaults to 2.
+ **servers**: A list of server address, port and weight (name:port:weight or ip:port:weight) for this server pool.


For example, the configuration file in [conf/nutcracker.yml](conf/nutcracker.yml), also shown below, configures 5 server pools with names - _alpha_, _beta_, _gamma_, _delta_ and omega. Clients that intend to send requests to one of the 10 servers in pool delta connect to port 22124 on 127.0.0.1. Clients that intend to send request to one of 2 servers in pool omega connect to unix path /tmp/gamma. Requests sent to pool alpha and omega have no timeout and might require timeout functionality to be implemented on the client side. On the other hand, requests sent to pool beta, gamma and delta timeout after 400 msec, 400 msec and 100 msec respectively when no response is received from the server. Of the 5 server pools, only pools alpha, gamma and delta are configured to use server ejection and hence are resilient to server failures. All the 5 server pools use ketama consistent hashing for key distribution with the key hasher for pools alpha, beta, gamma and delta set to fnv1a_64 while that for pool omega set to hsieh. Also only pool beta uses [nodes names](notes/recommendation.md#node-names-for-consistent-hashing) for consistent hashing, while pool alpha, gamma, delta and omega use 'host:port:weight' for consistent hashing. Finally, only pool alpha and beta can speak the redis protocol, while pool gamma, delta and omega speak memcached protocol.

    alpha:
      listen: 127.0.0.1:22121
      hash: fnv1a_64
      distribution: ketama
      auto_eject_hosts: true
      redis: true
      server_retry_timeout: 2000
      server_failure_limit: 1
      servers:
       - 127.0.0.1:6379:1

    beta:
      listen: 127.0.0.1:22122
      hash: fnv1a_64
      hash_tag: "{}"
      distribution: ketama
      auto_eject_hosts: false
      timeout: 400
      redis: true
      servers:
       - 127.0.0.1:6380:1 server1
       - 127.0.0.1:6381:1 server2
       - 127.0.0.1:6382:1 server3
       - 127.0.0.1:6383:1 server4

    gamma:
      listen: 127.0.0.1:22123
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

    delta:
      listen: 127.0.0.1:22124
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

    omega:
      listen: /tmp/gamma 0666
      hash: hsieh
      distribution: ketama
      auto_eject_hosts: false
      servers:
       - 127.0.0.1:11214:100000
       - 127.0.0.1:11215:1

Finally, to make writing a syntactically correct configuration file easier, twemproxy provides a command-line argument -t or --test-conf that can be used to test the YAML configuration file for any syntax error.

## Observability

Observability in twemproxy is through logs and stats.

Twemproxy exposes stats at the granularity of server pool and servers per pool through the stats monitoring port. The stats are essentially JSON formatted key-value pairs, with the keys corresponding to counter names. By default stats are exposed on port 22222 and aggregated every 30 seconds. Both these values can be configured on program start using the -c or --conf-file and -i or --stats-interval command-line arguments respectively. You can print the description of all stats exported by  using the -D or --describe-stats command-line argument.

    $ nutcracker --describe-stats

    pool stats:
      client_eof          "# eof on client connections"
      client_err          "# errors on client connections"
      client_connections  "# active client connections"
      server_ejects       "# times backend server was ejected"
      forward_error       "# times we encountered a forwarding error"
      fragments           "# fragments created from a multi-vector request"

    server stats:
      server_eof          "# eof on server connections"
      server_err          "# errors on server connections"
      server_timedout     "# timeouts on server connections"
      server_connections  "# active server connections"
      requests            "# requests"
      request_bytes       "total request bytes"
      responses           "# responses"
      response_bytes      "total response bytes"
      in_queue            "# requests in incoming queue"
      in_queue_bytes      "current request bytes in incoming queue"
      out_queue           "# requests in outgoing queue"
      out_queue_bytes     "current request bytes in outgoing queue"

Logging in twemproxy is only available when twemproxy is built with logging enabled. By default logs are written to stderr. Twemproxy can also be configured to write logs to a specific file through the -o or --output command-line argument. On a running twemproxy, we can turn log levels up and down by sending it SIGTTIN and SIGTTOU signals respectively and reopen log files by sending it SIGHUP signal.

## Pipelining

Twemproxy enables proxying multiple client connections onto one or few server connections. This architectural setup makes it ideal for pipelining requests and responses and hence saving on the round trip time.

For example, if twemproxy is proxying three client connections onto a single server and we get requests - 'get key\r\n', 'set key 0 0 3\r\nval\r\n' and 'delete key\r\n' on these three connections respectively, twemproxy would try to batch these requests and send them as a single message onto the server connection as 'get key\r\nset key 0 0 3\r\nval\r\ndelete key\r\n'.

Pipelining is the reason why twemproxy ends up doing better in terms of throughput even though it introduces an extra hop between the client and server.

## Deployment

If you are deploying twemproxy in production, you might consider reading through the [recommendation document](notes/recommendation.md) to understand the parameters you could tune in twemproxy to run it efficiently in the production environment.

## Packages

### Ubuntu

#### PPA Stable

https://launchpad.net/~twemproxy/+archive/ubuntu/stable

#### PPA Daily

https://launchpad.net/~twemproxy/+archive/ubuntu/daily

## Utils
+ [collectd-plugin](https://github.com/bewie/collectd-twemproxy)
+ [munin-plugin](https://github.com/eveiga/contrib/tree/nutcracker/plugins/nutcracker)
+ [twemproxy-ganglia-module](https://github.com/ganglia/gmond_python_modules/tree/master/twemproxy)
+ [nagios checks](https://github.com/wanelo/nagios-checks/blob/master/check_twemproxy)
+ [circonus](https://github.com/wanelo-chef/nad-checks/blob/master/recipes/twemproxy.rb)
+ [puppet module](https://github.com/wuakitv/puppet-twemproxy)
+ [nutcracker-web](https://github.com/kontera-technologies/nutcracker-web)
+ [redis-twemproxy agent](https://github.com/Stono/redis-twemproxy-agent)
+ [sensu-metrics](https://github.com/sensu-plugins/sensu-plugins-twemproxy/blob/master/bin/metrics-twemproxy.rb)
+ [redis-mgr](https://github.com/idning/redis-mgr)
+ [smitty for twemproxy failover](https://github.com/areina/smitty)
+ [Beholder, a Python agent for twemproxy failover](https://github.com/Serekh/beholder)
+ [chef cookbook](https://supermarket.getchef.com/cookbooks/twemproxy)
+ [twemsentinel] (https://github.com/yak0/twemsentinel)

## Companies using Twemproxy in Production
+ [Twitter](https://twitter.com/)
+ [Wikimedia](https://www.wikimedia.org/)
+ [Pinterest](http://pinterest.com/)
+ [Snapchat](http://www.snapchat.com/)
+ [Flickr](https://www.flickr.com)
+ [Yahoo!](https://www.yahoo.com)
+ [Tumblr](https://www.tumblr.com/)
+ [Vine](http://vine.co/)
+ [Wayfair](http://www.wayfair.com/)
+ [Kiip](http://www.kiip.me/)
+ [Wuaki.tv](https://wuaki.tv/)
+ [Wanelo](http://wanelo.com/)
+ [Kontera](http://kontera.com/)
+ [Bright](http://www.bright.com/)
+ [56.com](http://www.56.com/)
+ [Digg](http://digg.com/)
+ [Gawkermedia](http://advertising.gawker.com/)
+ [3scale.net](http://3scale.net)
+ [Ooyala](http://www.ooyala.com)
+ [Twitch](http://twitch.tv)
+ [Socrata](http://www.socrata.com/)
+ [Hootsuite](http://hootsuite.com/)
+ [Trivago](http://www.trivago.com/)
+ [Machinezone](http://www.machinezone.com)
+ [Path](https://path.com)
+ [AOL](http://engineering.aol.com/)
+ [Soysuper](https://soysuper.com/)
+ [Vinted](http://vinted.com/)
+ [Poshmark](https://poshmark.com/)
+ [FanDuel](https://www.fanduel.com/)
+ [Bloomreach](http://bloomreach.com/)
+ [Hootsuite](https://hootsuite.com)
+ [Tradesy](https://www.tradesy.com/)
+ [Uber](http://uber.com) ([details](http://highscalability.com/blog/2015/9/14/how-uber-scales-their-real-time-market-platform.html))
+ [Greta](https://greta.io/)

## Issues and Support

Have a bug or a question? Please create an issue here on GitHub!

https://github.com/twitter/twemproxy/issues

## Committers

* Manju Rajashekhar ([@manju](https://twitter.com/manju))
* Lin Yang ([@idning](https://github.com/idning))

Thank you to all of our [contributors](https://github.com/twitter/twemproxy/graphs/contributors)!

## License

Copyright 2012 Twitter, Inc.

Licensed under the Apache License, Version 2.0: http://www.apache.org/licenses/LICENSE-2.0
