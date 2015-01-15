## Tarantool handler notes

### Configuration

A Tarantool pool can be configured by following the general [configuration guidelines](../README.md#configuration) and using the `protocol: tarantool` configuration directive. For example, the following configuration file implements a Tarantool pool with the name _alpha_ on port 22121 on 127.0.0.1 with 3 servers on ports 3301, 3302 and 3303 on the same address, a 400 msec server timeout, _crc32_ as the hash algorithm and _modula_ as the distribution algorithm:

```
alpha:
  listen: 127.0.0.1:22121
  hash: crc32
  distribution: modula
  protocol: tarantool
  timeout: 400
  servers:
   - 127.0.0.1:3301:1
   - 127.0.0.1:3302:1
   - 127.0.0.1:3303:1
```

### Request length

The maximum length of a client request is the configured mbuf size (the `--mbuf-size=N` option) minus mbuf metadata overhead (48 bytes on a 64-bit machine). Server replies can span multiple mbufs.

### Field types

twemproxy is tailored to text protocols, i.e. when requests (including keys) are represented as text strings. Tarantool uses a binary protocol, which puts certain restrictions on how numeric keys are encoded. When a request contains a binary-encoded numeric key, it is handled as a binary string without any parsing or conversions. Sharding on such keys is consistent as long as all clients use the same encoding rules to convert numeric values to their [MsgPack](http://msgpack.org/) representations. This can be achieved by following the MsgPack specification which recommends encoding data in the smallest possible number of bytes.

String keys in requests are parsed and hashed natively without encoding restrictions.

### Multi-part indexes

Tarantool supports multi-part indexes. If a multi-part key is used in a request, only the first key part is considered a sharding key (i.e. used to calculate the hash value) by the Tarantool handler.

### Authentication

Authentication is currently not supported. twemproxy connects to servers as a guest user. A randomly-generated salt value is sent to clients in a greeting message on connect, but authentication requests from clients are always replied with an OK packet.

### CALL requests

For CALL requests the Tarantool handler uses the first argument as a sharding key. Currently this is not configurable.

### INSERT requests

As INSERT requests contain only a tuple to be inserted, but not a key value, the Tarantool handler assumes the first value in the tuple is the sharding key. Currently this is not configurable.

### UPDATE requests

Tarantool allows updating a primary key value in an UPDATE request. The Tarantool handler does not re-shard the record according to the new sharding key value in such a case.

### PING requests

PING requests are replied by twemproxy itself without forwarding to server(s).

### Out-of-order replies

Tarantool can send out-of-order replies. Those are handled correctly by twemproxy by using the ‘sync’ value to match a response to the corresponding client request.


