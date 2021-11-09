## Memcache Command Support

### Request

- Twemproxy implements only the memached ASCII commands
- Binary commands are currently unsupported

#### Ascii Storage Command

    +-------------------+------------+--------------------------------------------------------------------------+
    |      Command      | Supported? | Format                                                                   |
    +-------------------+------------+--------------------------------------------------------------------------+
    |        set        |    Yes     | set <key> <flags> <expiry> <datalen> [noreply]\r\n<data>\r\n             |
    +-------------------+------------+--------------------------------------------------------------------------+
    |        add        |    Yes     | add <key> <flags> <expiry> <datalen> [noreply]\r\n<data>\r\n             |
    +-------------------+------------+--------------------------------------------------------------------------+
    |      replace      |    Yes     | replace <key> <flags> <expiry> <datalen> [noreply]\r\n<data>\r\n         |
    +-------------------+------------+--------------------------------------------------------------------------+
    |      append       |    Yes     | append <key> <flags> <expiry> <datalen> [noreply]\r\n<data>\r\n          |
    +-------------------+------------+--------------------------------------------------------------------------+
    |      prepend      |    Yes     | prepend <key> <flags> <expiry> <datalen> [noreply]\r\n<data>\r\n         |
    +-------------------+------------+--------------------------------------------------------------------------+
    |       cas         |    Yes     | cas <key> <flags> <expiry> <datalen> <cas> [noreply]\r\n<data>\r\n       |
    +-------------------+------------+--------------------------------------------------------------------------+

* Where,
  * <flags>   - uint32_t : data specific client side flags
  * <expiry>  - uint32_t : expiration time (in seconds)
  * <datalen> - uint32_t : size of the data (in bytes)
  * <data>    - uint8_t[]: data block
  * <cas>     - uint64_t

#### Ascii Retrieval Command

    +-------------------+------------+--------------------------------------------------------------------------+
    |      Command      | Supported? | Format                                                                   |
    +-------------------+------------+--------------------------------------------------------------------------+
    |        get        |    Yes     | get <key> [<key>]+\r\n                                                   |
    +-------------------+------------+--------------------------------------------------------------------------+
    |        gets       |    Yes     | gets <key> [<key>]+\r\n                                                  |
    +-------------------+------------+--------------------------------------------------------------------------+

#### Ascii Delete

    +-------------------+------------+--------------------------------------------------------------------------+
    |      Command      | Supported? | Format                                                                   |
    +-------------------+------------+--------------------------------------------------------------------------+
    |        delete     |    Yes     | delete <key> [noreply]\r\n                                               |
    +-------------------+------------+--------------------------------------------------------------------------+

#### Ascii Arithmetic Command

    +-------------------+------------+--------------------------------------------------------------------------+
    |      Command      | Supported? | Format                                                                   |
    +-------------------+------------+--------------------------------------------------------------------------+
    |        incr       |    Yes     | incr <key> <value> [noreply]\r\n                                         |
    +-------------------+------------+--------------------------------------------------------------------------+
    |        decr       |    Yes     | decr <key> <value> [noreply]\r\n                                         |
    +-------------------+------------+--------------------------------------------------------------------------+

* Where,
  * <value> - uint64_t

#### Ascii Misc Command

    +-------------------+------------+--------------------------------------------------------------------------+
    |      Command      | Supported? | Format                                                                   |
    +-------------------+------------+--------------------------------------------------------------------------+
    |       touch       |    Yes     | touch <key> <expiry>[noreply]\r\n                                        |
    +-------------------+------------+--------------------------------------------------------------------------+
    |        gat        |  Planned   | gat <expiry> <key>+\r\n                                                  |
    +-------------------+------------+--------------------------------------------------------------------------+
    |        gats       |  Planned   | gats <expiry> <key>+\r\n                                                 |
    +-------------------+------------+--------------------------------------------------------------------------+
    |        quit       |    Yes     | quit\r\n                                                                 |
    +-------------------+------------+--------------------------------------------------------------------------+
    |      flush_all    |    No      | flush_all [<delay>] [noreply]\r\n                                        |
    +-------------------+------------+--------------------------------------------------------------------------+
    |      version      |    Yes     | version\r\n                                                              |
    +-------------------+------------+--------------------------------------------------------------------------+
    |      verbosity    |    No      | verbosity <num> [noreply]\r\n                                            |
    +-------------------+------------+--------------------------------------------------------------------------+
    |       stats       |    No      | stats\r\n                                                                |
    +-------------------+------------+--------------------------------------------------------------------------+
    |       stats       |    No      | stats <args>\r\n                                                         |
    +-------------------+------------+--------------------------------------------------------------------------+

### Response

#### Error Responses

    ERROR\r\n
    CLIENT_ERROR [error]\r\n
    SERVER_ERROR [error]\r\n

    Where,
    - ERROR means client sent a non-existent command name
    - CLIENT_ERROR means that command sent by the client does not conform to the protocol
    - SERVER_ERROR means that there was an error on the server side that made processing of the command impossible

#### Storage Command Responses

    STORED\r\n
    NOT_STORED\r\n
    EXISTS\r\n
    NOT_FOUND\r\n

    Where,
    - STORED indicates success.
    - NOT_STORED indicates the data was not stored because condition for an add or replace wasn't met.
    - EXISTS indicates that the item you are trying to store with a cas has been modified since you last fetched it.
    - NOT_FOUND indicates that the item you are trying to store with a cas does not exist.

#### Delete Command Responses

    NOT_FOUND\r\n
    DELETED\r\n

#### Retrieval Responses

    END\r\n
    VALUE <key> <flags> <datalen> [<cas>]\r\n<data>\r\nEND\r\n
    VALUE <key> <flags> <datalen> [<cas>]\r\n<data>\r\n[VALUE <key> <flags> <datalen> [<cas>]\r\n<data>]+\r\nEND\r\n

#### Arithmetic Responses

    NOT_FOUND\r\n
    <value>\r\n

    Where,
    - <value> - uint64_t : new key value after incr or decr operation

#### Touch Command Responses

    NOT_FOUND\r\n
    TOUCHED\r\n

#### Statistics Response

    [STAT <name> <value>\r\n]+END\r\n

#### Misc Responses

    OK\r\n
    VERSION <version>\r\n

### Notes

- set always creates mapping irrespective of whether it is present on not.
- add, adds only if the mapping is not present
- replace, only replaces if the mapping is present
- append and prepend command ignore flags and expiry values
- noreply instructs the server to not send the reply even if there is an error.
- decr of 0 is 0, while incr of UINT64_MAX is 0
- maximum length of the key is 250 characters
- expiry of 0 means that item never expires, though it could be evicted from the cache
- non-zero expiry is either unix time (# seconds since 01/01/1970) or,
  offset in seconds from the current time (< 60 x 60 x 24 x 30 seconds = 30 days)
- expiry time is with respect to the server (not client)
- <datalen> can be zero and when it is, the <data> block is empty.
- Thoughts:
  - ascii protocol is easier to debug - think using strace or tcpdump to see
    protocol on the wire, Or using telnet or netcat or socat to build memcache
    requests and responses
    https://stackoverflow.com/questions/2525188/are-binary-protocols-dead
  - nutcracker will support the more efficient meta-text protocol after the protocol
    is marked as stable and memcached servers using it have had several releases.
  - https://news.ycombinator.com/item?id=1712788
