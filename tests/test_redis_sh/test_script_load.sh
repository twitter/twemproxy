#!/bin/bash

redis-cli -p 7379 SET a x
RET=`redis-cli -p 7379 GET a`
[ "$RET" != "x" ] && echo "redis not running" && exit -1
RET=`redis-cli -p 7380 GET a`
[ "$RET" = "x" ] && echo "replica setting wrongly applied" && exit -1

SHA1=`redis-cli script load 'return "hello"'`
echo "SHA1:$SHA1"
RET=`redis-cli -p 7379 evalsha $SHA1 1 key`
[ "$RET" != "hello" ] && echo "script not correctly loaded at 7379" && exit -1
RET=`redis-cli -p 7380 evalsha $SHA1 1 key`
[ "$RET" != "hello" ] && echo "script not correctly loaded at 7380" && exit -1

echo "success"
