#!/bin/sh

socatopt="-t 1 -T 1 -b 16384"

val=`echo 6^6^6 | bc`
val=`printf "%s" "${val}"`
vallen=`printf "%s" "${val}" | wc -c`
set_command=""
set_commands=""

# build
for i in `seq 1 64`; do
    if [ `expr $i % 2` -eq "0" ]; then
        key="foo"
    else
        key="bar"
    fi
    key=`printf "%s%d" "${key}" "${i}"`

    set_command="set ${key} 0 0 ${vallen}\r\n${val}\r\n"
    set_commands=`printf "%s%s" "${set_commands}" "${set_command}"`
done

printf "%b" "$set_commands" > /tmp/socat.input

# write
for i in `seq 1 16`; do
    cat /tmp/socat.input | socat ${socatopt} - TCP:localhost:22123,nodelay,shut-down,nonblock=1 &
done
