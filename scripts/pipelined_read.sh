#!/bin/sh

socatopt="-t 4 -T 4 -b 8193 -d -d "

get_commands=""

# build
for i in `seq 1 128`; do
    if [ `expr $i % 2` -eq "0" ]; then
        key="foo"
    else
        key="bar"
    fi
    key=`printf "%s%d" "${key}" "${i}"`

    get_command="get ${key}\r\n"
    get_commands=`printf "%s%s" "${get_commands}" "${get_command}"`
done

# read
for i in `seq 1 64`; do
    printf "%b" "$get_commands" | socat ${socatopt} - TCP:localhost:22123,nodelay,shut-none,nonblock=1 1> /dev/null 2>&1 &
done
