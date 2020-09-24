#!/bin/bash
#file   : travis.sh
#author : ning
#date   : 2014-05-10 16:54:43

cp `which nutcracker`  tests/_binaries/
cp `which redis-server` tests/_binaries/
cp `which redis-cli` tests/_binaries/
cp `which memcached` tests/_binaries/

#run test
# export T_LOGFILE=-
# export T_VERBOSE=10
redis-server --version
nosetests -w tests/ --nologcapture -x -v