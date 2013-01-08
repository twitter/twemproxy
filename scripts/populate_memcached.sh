#!/bin/sh

port=22123
socatopt="-t 1 -T 1 -b 65537"

val=`echo 6^6^6 | bc`
val=`printf "%s\r\n" "${val}"`
vallen=`printf "%s" "${val}" | wc -c`
set_command=""

# build
for i in `seq 1 512`; do
    if [ `expr $i % 2` -eq "0" ]; then
        key="foo"
    else
        key="bar"
    fi
    key=`printf "%s%d" "${key}" "${i}"`

    set_command="set ${key} 0 0 ${vallen}\r\n${val}\r\n"

    printf "%b" "$set_command" | socat ${socatopt} - TCP:localhost:${port},nodelay,shut-down,nonblock=1 &
done

