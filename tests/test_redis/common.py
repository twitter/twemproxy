#!/usr/bin/env python3

import os
import sys
import redis
import time

PWD = os.path.dirname(os.path.realpath(__file__))
WORKDIR = os.path.join(PWD,'../')
sys.path.append(os.path.join(WORKDIR,'lib/'))
sys.path.append(os.path.join(WORKDIR,'conf/'))

import conf

from server_modules import *
from utils import *

CLUSTER_NAME = 'ntest'
nc_verbose = int(getenv('T_VERBOSE', 5))
mbuf = int(getenv('T_MBUF', 512))
large = int(getenv('T_LARGE', 1000))

all_redis = [
        RedisServer('127.0.0.1', 2100, '/tmp/r/redis-2100/', CLUSTER_NAME, 'redis-2100'),
        RedisServer('127.0.0.1', 2101, '/tmp/r/redis-2101/', CLUSTER_NAME, 'redis-2101'),
        ]

nc = NutCracker('127.0.0.1', 4100, '/tmp/r/nutcracker-4100', CLUSTER_NAME,
                all_redis, mbuf=mbuf, verbose=nc_verbose)

def setup():
    print('setup(mbuf=%s, verbose=%s)' %(mbuf, nc_verbose))
    for r in all_redis + [nc]:
        r.clean()
        r.deploy()
        r.stop()
        r.start()

def teardown():
    all_alive = True
    for r in all_redis + [nc]:
        if not r._alive():
            all_alive = False
        r.stop()
    assert(all_alive)

default_kv = {b'kkk-%d' % i : b'vvv-%d' % i for i in range(10)}

def getconn():
    for r in all_redis:
        c = redis.Redis(r.host(), r.port(), decode_responses=True)
        c.flushdb()

    r = redis.Redis(nc.host(), nc.port(), decode_responses=True)
    return r

def getconn_no_decode():
    for r in all_redis:
        c = redis.Redis(r.host(), r.port())
        c.flushdb()

    r = redis.Redis(nc.host(), nc.port())
    return r

