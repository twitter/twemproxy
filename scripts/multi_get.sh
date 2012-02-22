#!/bin/sh

port=22123
socatopt="-t 20 -T 20 -b 8193 -d -d "
key=""
keys=""
get_command=""

# build
for i in `seq 1 512`; do
    if [ `expr $i % 2` -eq "0" ]; then
        key="foo"
    else
        key="bar"
    fi
    key=`printf "%s%d" "${key}" "${i}"`
    keys=`printf "%s %s" "${keys}" "${key}"`
done

get_command="get ${keys}\r\n"
printf "%b" "$get_command"

# read
for i in `seq 1 16`; do
    printf "%b" "${get_command}" | socat ${socatopt} - TCP:localhost:${port},nodelay,shut-none,nonblock=1 1> /dev/null 2>&1 &
done
