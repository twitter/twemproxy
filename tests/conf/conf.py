#coding: utf-8

import os
import sys

PWD = os.path.dirname(os.path.realpath(__file__))
WORKDIR = os.path.join(PWD,  '../')

BINARYS = {
    'REDIS_SERVER_BINS'   : os.path.join(WORKDIR, '_binaries/redis-*'),
    'REDIS_CLI'           : os.path.join(WORKDIR, '_binaries/redis-cli'),
    'MEMCACHED_BINS'      : os.path.join(WORKDIR, '_binaries/memcached'),
    'NUTCRACKER_BINS'     : os.path.join(WORKDIR, '_binaries/nutcracker'),
}

