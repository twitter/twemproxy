#!/usr/bin/env python
#coding: utf-8
#file   : test_sentinel.py
#author : andyqzb
#date   : 2015-07-18 15:20:21

import os
import sys
import redis

PWD = os.path.dirname(os.path.realpath(__file__))
WORKDIR = os.path.join(PWD,'../')
sys.path.append(os.path.join(WORKDIR,'lib/'))
sys.path.append(os.path.join(WORKDIR,'conf/'))

import conf

from server_modules import *
from utils import *
from nose import with_setup

CLUSTER_NAME = 'ntest'
nc_verbose = int(getenv('T_VERBOSE', 11))
mbuf = int(getenv('T_MBUF', 512))
large = int(getenv('T_LARGE', 1000))
quorum = int(getenv('T_QUORUM', 2))
down_time_msec = int(getenv('T_DOWM_TIME', 2000))

T_FAILOVER_SEC = 3
T_ADD_SLAVE_SEC = 12
T_SERVER_RETRY_SEC = 2
T_LOOP_COUNT = 20

redis_masters = [
        RedisServer('127.0.0.1', 2100, '/tmp/r/redis-2100', CLUSTER_NAME, 'server1'),
        RedisServer('127.0.0.1', 2101, '/tmp/r/redis-2101', CLUSTER_NAME, 'server2'),
    ]

redis_slaves = [
        RedisServer('127.0.0.1', 3100, '/tmp/r/redis-3100', CLUSTER_NAME, 'server1'),
        RedisServer('127.0.0.1', 3101, '/tmp/r/redis-3101', CLUSTER_NAME, 'server2'),
    ]

redis_sentinels = [
        RedisSentinel('127.0.0.1', 4100, '/tmp/r/sentinel-4100', CLUSTER_NAME,
                      'sentinel-4100', redis_masters, quorum, down_time_msec),
        RedisSentinel('127.0.0.1', 4101, '/tmp/r/sentinel-4101', CLUSTER_NAME,
                      'sentinel-4101', redis_masters, quorum, down_time_msec),
        RedisSentinel('127.0.0.1', 4102, '/tmp/r/sentinel-4102', CLUSTER_NAME,
                      'sentinel-4102', redis_masters, quorum, down_time_msec),
    ]

ncs = [
        #nc with right conf
        NutCracker('127.0.0.1', 5100, '/tmp/r/nutcracker-5100', CLUSTER_NAME,
                   redis_masters, mbuf=mbuf, verbose=nc_verbose, sentinels=redis_sentinels),
        #nc with error redis conf
        NutCracker('127.0.0.1', 5101, '/tmp/r/nutcracker-5101', CLUSTER_NAME,
                   redis_slaves, mbuf=mbuf, verbose=nc_verbose, sentinels=redis_sentinels)
    ]

def _setup():
    print('setup(mbuf=%s, verbose=%s)' %(mbuf, nc_verbose))
    for r in redis_masters + redis_slaves + redis_sentinels + ncs:
        r.deploy()
        r.stop()
        r.start()

    for i, r in enumerate(redis_slaves):
        r.slaveof(redis_masters[i].host(), redis_masters[i].port())

    #wait for the slave sync with master and added to the sentinel
    time.sleep(T_ADD_SLAVE_SEC)

def _teardown():
    for r in redis_masters + redis_slaves + redis_sentinels + ncs:
        if r._alive():
            r.stop()

@with_setup(_setup, _teardown)
def test_cmd_failover():
    r = redis.Redis(ncs[0].host(), ncs[0].port())

    keys = []
    values = []
    for i in range(0, T_LOOP_COUNT):
        keys.append('test_key:%d' % i)
        values.append('test_value:%d' % i)
        assert(r.set(keys[i], values[i]))

    for master in redis_masters:
        redis_sentinels[0].failover(master.args['server_name'])

    #wait for the sentinel do the failover
    time.sleep(T_FAILOVER_SEC)

    fail_count = 0
    for i in range(0, len(keys)):
        try:
            assert(r.set(keys[i], values[i]))
        except:
            fail_count += 1
    #twemproxy just mark the old conn done, so after the switch,
    #the first request to the old conn will got a conn error
    assert(fail_count == len(redis_masters))

@with_setup(_setup, _teardown)
def test_redis_down():
    r = redis.Redis(ncs[0].host(), ncs[0].port())

    keys = []
    values = []
    for i in range(0, T_LOOP_COUNT):
        keys.append('test_key:%d' % i)
        values.append('test_value:%d' % i)
        assert(r.set(keys[i], values[i]))

    for master in redis_masters:
        master.stop()

    #wait the sentinel mark the master down and done the failover
    time.sleep(down_time_msec / 1000 + T_FAILOVER_SEC)

    for i in range(0, len(keys)):
        #all conn was closed when the old master is stopped.
        #so twemproxy don't have the old conn problem.
        assert(r.set(keys[i], values[i]))

@with_setup(_setup, _teardown)
def test_sentinel_down():
    r = redis.Redis(ncs[0].host(), ncs[0].port())

    keys = []
    values = []
    for i in range(0, T_LOOP_COUNT):
        keys.append('test_key:%d' % i)
        values.append('test_value:%d' % i)
        assert(r.set(keys[i], values[i]))

    #the twemproxy use the first sentinel in the conf, so just stop the first.
    redis_sentinels[0].stop()
    #twemproxy will reconnect a new sentinel after 2 sec(reuse the server_retry_timeout).
    #Besides, the reconnect code is in the server_pool_conn, so we should send a request to twemproxy
    #to let it reconnect a new sentinel after the server_retry_timeout.
    time.sleep(T_SERVER_RETRY_SEC + 0.1)
    assert(r.set(keys[0], values[0]))

    for master in redis_masters:
        master.stop()

    time.sleep(down_time_msec / 1000 + T_FAILOVER_SEC)

    for i in range(0, len(keys)):
        assert(r.set(keys[i], values[i]))

@with_setup(_setup, _teardown)
def test_error_conf():
    #get conn from error conf nc
    r = redis.Redis(ncs[1].host(), ncs[1].port())

    fail_count = 0
    for i in range(0, T_LOOP_COUNT):
        key = 'test_key:%d' % i
        value = 'test_value:%d' % i
        try:
            assert(r.set(key, value))
        except:
            fail_count += 1
    #default preconnect is true, so the first request to each redis will
    #hit the old conn.
    assert(fail_count == len(redis_masters))
