If you are deploying nutcracker in your production environment, here are a few recommendations that might be worth considering.

## Log Level

By default debug logging is disabled in nutcracker. However, it is worthwhile running nutcracker with debug logging enabled and verbosity level set to LOG_INFO (-v 6 or --verbosity=6). This in reality does not add much overhead as you only pay the cost of checking an if condition for every log line encountered during the run time.

At LOG_INFO level, nutcracker logs the life cycle of every client and server connection and important events like the server being ejected from the hash ring and so on. Eg.

    [Thu Aug  2 00:03:09 2012] nc_proxy.c:336 accepted c 7 on p 6 from '127.0.0.1:54009'
    [Thu Aug  2 00:03:09 2012] nc_server.c:528 connected on s 8 to server '127.0.0.1:11211:1'
    [Thu Aug  2 00:03:09 2012] nc_core.c:270 req 1 on s 8 timedout
    [Thu Aug  2 00:03:09 2012] nc_core.c:207 close s 8 '127.0.0.1:11211' on event 0004 eof 0 done 0 rb 0 sb 20: Connection timed out
    [Thu Aug  2 00:03:09 2012] nc_server.c:406 close s 8 schedule error for req 1 len 20 type 5 from c 7: Connection timed out
    [Thu Aug  2 00:03:09 2012] nc_server.c:281 update pool 0 'alpha' to delete server '127.0.0.1:11211:1' for next 2 secs
    [Thu Aug  2 00:03:10 2012] nc_connection.c:314 recv on sd 7 eof rb 20 sb 35
    [Thu Aug  2 00:03:10 2012] nc_request.c:334 c 7 is done
    [Thu Aug  2 00:03:10 2012] nc_core.c:207 close c 7 '127.0.0.1:54009' on event 0001 eof 1 done 1 rb 20 sb 35
    [Thu Aug  2 00:03:11 2012] nc_proxy.c:336 accepted c 7 on p 6 from '127.0.0.1:54011'
    [Thu Aug  2 00:03:11 2012] nc_server.c:528 connected on s 8 to server '127.0.0.1:11212:1'
    [Thu Aug  2 00:03:12 2012] nc_connection.c:314 recv on sd 7 eof rb 20 sb 8
    [Thu Aug  2 00:03:12 2012] nc_request.c:334 c 7 is done
    [Thu Aug  2 00:03:12 2012] nc_core.c:207 close c 7 '127.0.0.1:54011' on event 0001 eof 1 done 1 rb 20 sb 8

To enable debug logging, you have to compile nutcracker with logging enabled using --enable-debug=log configure option.

## Liveness

Failures are a fact of life, especially when things are distributed. To be resilient against failures, it is recommended that you configure the following keys for every server pool. Eg:

    resilient_pool:
      auto_eject_hosts: true
      server_retry_timeout: 30000
      server_failure_limit: 3

Enabling `auto_eject_hosts:` ensures that a dead server can be ejected out of the hash ring after `server_failure_limit:` consecutive failures have been encountered on that said server. A non-zero `server_retry_timeout:` ensures that we don't incorrectly mark a server as dead forever especially when the failures were really transient. The combination of `server_retry_timeout:` and `server_failure_limit:` controls the tradeoff between resiliency to permanent and transient failures.

Note that an ejected server will not be included in the hash ring for any requests until the retry timeout passes. This will lead to data partitioning as keys originally on the ejected server will now be written to a server still in the pool.

To ensure that requests always succeed in the face of server ejections (`auto_eject_hosts:` is enabled), some form of retry must be implemented at the client layer since nutcracker itself does not retry a request. This client-side retry count must be greater than `server_failure_limit:` value, which ensures that the original request has a chance to make it to a live server.

## Timeout

It is always a good idea to configure nutcracker `timeout:` for every server pool, rather than purely relying on client-side timeouts. Eg:

    resilient_pool_with_timeout:
      auto_eject_hosts: true
      server_retry_timeout: 30000
      server_failure_limit: 3
      timeout: 400

Relying only on client-side timeouts has the adverse effect of the original request having timedout on the client to proxy connection, but still pending and outstanding on the proxy to server connection. This further gets exacerbated when client retries the original request.

By default, nutcracker waits indefinitely for any request sent to the server. However, when `timeout:` key is configured, a requests for which no response is received from the server in `timeout:` msec is timedout and an error response `SERVER_ERROR Connection timed out\r\n` is sent back to the client.

## Error Response

Whenever a request encounters failure on a server we usually send to the client a response with the general form - `SERVER_ERROR <errno description>\r\n` (memcached) or `-ERR <errno description>` (redis).

For example, when a memcache server is down, this error response is usually:

+ `SERVER_ERROR Connection refused\r\n` or,
+ `SERVER_ERROR Connection reset by peer\r\n`

When the request timedout, the response is usually:

+ `SERVER_ERROR Connection timed out\r\n`

Seeing a `SERVER_ERROR` or `-ERR` response should be considered as a transient failure by a client which makes the original request an ideal candidate for a retry.

## read, writev and mbuf

All memory for incoming requests and outgoing responses is allocated in mbuf. Mbuf enables zero copy for requests and responses flowing through the proxy. By default an mbuf is 16K bytes in size and this value can be tuned between 512 and 65K bytes using -m or --mbuf-size=N argument. Every connection has at least one mbuf allocated to it. This means that the number of concurrent connections nutcracker can support is dependent on the mbuf size. A small mbuf allows us to handle more connections, while a large mbuf allows us to read and write more data to and from kernel socket buffers.

If nutcracker is meant to handle a large number of concurrent client connections, you should set the mbuf size to 512 or 1K bytes.

## Maximum Key Length
The memcache ascii protocol [specification](notes/memcache.txt) limits the maximum length of the key to 250 characters. The key should not include whitespace, or '\r' or '\n' character. For redis, we have no such limitation. However, nutcracker requires the key to be stored in a contiguous memory region. Since all requests and responses in nutcracker are stored in mbuf, the maximum length of the redis key is limited by the size of the maximum available space for data in mbuf (mbuf_data_size()). This means that if you want your redis instances to handle large keys, you might want to choose large mbuf size set using -m or --mbuf-size=N command-line argument.

## Node Names for Consistent Hashing

The server cluster in twemproxy can either be specified as list strings in format 'host:port:weight' or 'host:port:weight name'.

    servers:
     - 127.0.0.1:6379:1
     - 127.0.0.1:6380:1
     - 127.0.0.1:6381:1
     - 127.0.0.1:6382:1

Or,

    servers:
     - 127.0.0.1:6379:1 server1
     - 127.0.0.1:6380:1 server2
     - 127.0.0.1:6381:1 server3
     - 127.0.0.1:6382:1 server4


In the former configuration, keys are mapped **directly** to **'host:port:weight'** triplet and in the latter they are mapped to **node names** which are then mapped to nodes i.e. host:port pair. The latter configuration gives us the freedom to relocate nodes to a different server without disturbing the hash ring and hence makes this configuration ideal when auto_eject_hosts is set to false. See [issue 25](https://github.com/twitter/twemproxy/issues/25) for details.

Note that when using node names for consistent hashing, twemproxy ignores the weight value in the 'host:port:weight name' format string.

## Hash Tags

[Hash Tags](http://antirez.com/post/redis-presharding.html) enables you to use part of the key for calculating the hash. When the hash tag is present, we use part of the key within the tag as the key to be used for consistent hashing. Otherwise, we use the full key as is. Hash tags enable you to map different keys to the same server as long as the part of the key within the tag is the same.

For example, the configuration of server pool _beta_, aslo shown below, specifies a two character hash_tag string - "{}". This means that keys "user:{user1}:ids" and "user:{user1}:tweets" map to the same server because we compute the hash on "user1". For a key like "user:user1:ids", we use the entire string "user:user1:ids" to compute the hash and it may map to a different server.

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
       
       
## Graphing Cache-pool State

When running nutcracker in production, you often would like to know the list of live and ejected servers at any given time. You can easily answer this question, by generating a time series graph of live and/or dead servers that are part of any cache pool. To do this your graphing client must collect the following stats exposed by nutcracker:

- **server_eof** which is incremented when server closes the connection normally which should not happen because we use persistent connections.
- **server_timedout** is incremented when the connection / request to server timedout.
- **server_err** is incremented for any other kinds of errors.

So, on a given server, the cumulative number of times a server is ejected can be computed as:

```c
(server_err + server_timedout + server_eof) / server_failure_limit
```

A diff of the above value between two successive time intervals would generate a nice timeseries graph for ejected servers.
