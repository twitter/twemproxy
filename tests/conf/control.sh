#!/bin/bash

start() 
{
    stop
    ulimit -c unlimited

    pushd . > /dev/null

    cd `dirname $$0`
    ${startcmd}
    popd
}

stop() 
{
    pkill -9 -f '${runcmd}'
}

case C"$$1" in
    C)
        echo "Usage: $$0 {start|stop}"
        ;; 
    Cstart)
        start
        echo "Done!"
        ;;
    Cstop)
        stop
        echo "Done!"
        ;;
    C*)
        echo "Usage: $$0 {start|stop}"
        ;;
esac



