#!/bin/bash

REDIS=`which redis-server`
mkdir dir/7379
$REDIS ./conf/7379.conf
mkdir dir/7380
$REDIS ./conf/7380.conf

NC=`which nutcracker`
$NC -c ./conf/nutcracker.yml &

sleep 3