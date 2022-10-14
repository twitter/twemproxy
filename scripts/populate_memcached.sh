#!/bin/sh

port=22123
socatopt="-t 1 -T 1 -b 65537"

val=`tr -dc A-z-0-9 </dev/urandom |  head -c 6`
val=`printf "%s\r\n" "${val}"`
vallen=`printf "%s" "${val}" | wc -c`
set_command=""

# build
for i in `seq 1 1000`; do
    if [ `expr $i % 2` -eq "0" ]; then
        key="foo"
    else
        key="bar"
    fi
    key=`printf "%s%d" "${key}" "${i}"`

    set_command="set ${key} 0 0 ${vallen}\r\n${val}\r\n"

    printf "%b" "$set_command" | socat ${socatopt} - TCP:10.27.30.55:${port},nodelay,shut-down,nonblock=1
    printf "%b" "get ${key}\r\n" | socat ${socatopt} - TCP:10.27.30.55:${port},nodelay,shut-down,nonblock=1
    trap "echo 'interrupted'; exit;" SIGINT SIGTERM
done
