#!/bin/bash

set -eu

killall tarantool || true
killall nutcracker || true

port1=3301
port2=3302
port3=3303

wd1=$(mktemp -d -t tnt.XXXXXX)
wd2=$(mktemp -d -t tnt.XXXXXX)
wd3=$(mktemp -d -t tnt.XXXXXX)

TARANTOOL_CONF="
box.cfg({
listen=os.getenv('TARANTOOL_PORT'),
work_dir=os.getenv('TARANTOOL_WD')
})
s = box.schema.space.create('test')
i = s:create_index('primary', {parts = {1, 'STR'}})
box.schema.user.grant('guest', 'execute,read,write', 'universe')
json = require('json')
uuid = require('uuid')
"

TWEMPROXY_CONF="
alpha:
  listen: 127.0.0.1:22121
  hash: crc32
  distribution: modula
  auto_eject_hosts: true
  protocol: tarantool
  timeout: 400
  server_retry_timeout: 2000
  server_failure_limit: 1
  servers:
   - 127.0.0.1:3301:1
   - 127.0.0.1:3302:1
   - 127.0.0.1:3303:1
"

TARANTOOL_PORT=$port1 TARANTOOL_WD=$wd1 tarantool <(echo $TARANTOOL_CONF) >/dev/null 2>&1 &
TARANTOOL_PORT=$port2 TARANTOOL_WD=$wd2 tarantool <(echo $TARANTOOL_CONF) >/dev/null 2>&1 &
TARANTOOL_PORT=$port3 TARANTOOL_WD=$wd3 tarantool <(echo $TARANTOOL_CONF) >/dev/null 2>&1 &

twemproxy_conf=$(mktemp -t twemproxy.XXXXXX)
echo "$TWEMPROXY_CONF" >$twemproxy_conf

twemproxy_log=$(mktemp -t twemproxy_log.XXXXXX)
echo "twemproxy log can be found in $twemproxy_log"

nutcracker -c $twemproxy_conf --verbose=10 --mbuf-size=512 >$twemproxy_log 2>&1 &
