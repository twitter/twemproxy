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

## Error response

Whenever a request encounters failure on a server we usually send a response with a general form `SERVER_ERROR <errno description>\r\n` to the client. For example, when a server is down, this error response is usually `SERVER_ERROR Connection refused\r\n` or `SERVER_ERROR Connection reset by peer\r\n` and when the request timedout, the response is `SERVER_ERROR Connection timed out\r\n`.

Seeing a `SERVER_ERROR` response should be considered as a transient failure by a client which makes the original request an ideal candidate for a retry.

## read, writev and mbuf

All memory for incoming requests and outgoing responses is allocated in mbuf. Mbuf enables zero copy for requests and responses flowing through the proxy. By default an mbuf is 16K bytes in size and this value can be tuned between 512 and 65K bytes using -m or --mbuf-size=N argument. Every connection has at least one mbuf allocated to it. This means that the number of concurrent connections nutcracker can support is dependent on the mbuf size. A small mbuf allows us to handle more connections, while a large mbuf allows us to read and write more data to and from kernel socket buffers.

If nutcracker is meant to handle a large number of concurrent client connections, you should set the mbuf size to 512 or 1K bytes.
