#!/bin/bash
#file   : buld.sh
#author : perrynzhou
#date   : 2018-05-10 16:54:43
autoreconf -fvi
CFLAGS="-ggdb3 -O0" ./configure --enable-debug=full
make