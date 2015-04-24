#!/bin/bash
#file   : travis.sh
#author : ning
#date   : 2014-05-10 16:54:43

#install
#sudo pip install redis
#sudo pip install nose

#pip install git+https://github.com/andymccurdy/redis-py.git@2.9.0
#pip install git+https://github.com/idning/python-memcached.git#egg=memcache

#build twemproxy
CFLAGS="-ggdb3 -O0" autoreconf -fvi && ./configure --enable-debug=log && make 

cp  src/nutcracker  tests/_binaries/
cp `which redis-server` tests/_binaries/
cp `which redis-cli` tests/_binaries/
cp `which memcached` tests/_binaries/

#run test
cd tests/ && nosetests --nologcapture -x 

