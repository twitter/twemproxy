#!/usr/bin/env python
#coding: utf-8

from common import *

all_redis = [
    RedisServer('127.0.0.1', 2100, '/tmp/r/redis-2100/',
                CLUSTER_NAME, 'redis-2100', auth = 'hellopasswd'),
    RedisServer('127.0.0.1', 2101, '/tmp/r/redis-2101/',
                CLUSTER_NAME, 'redis-2101', auth = 'hellopasswd'),
]

nc = NutCracker('127.0.0.1', 4100, '/tmp/r/nutcracker-4100', CLUSTER_NAME,
                all_redis, mbuf=mbuf, verbose=nc_verbose,
                redis_auth = 'hellopasswd')

nc_badpass = NutCracker('127.0.0.1', 4101, '/tmp/r/nutcracker-4101', CLUSTER_NAME,
                        all_redis, mbuf=mbuf, verbose=nc_verbose,
                        redis_auth = 'badpasswd')
nc_nopass = NutCracker('127.0.0.1', 4102, '/tmp/r/nutcracker-4102', CLUSTER_NAME,
                       all_redis, mbuf=mbuf, verbose=nc_verbose)

def setup():
    print 'setup(mbuf=%s, verbose=%s)' %(mbuf, nc_verbose)
    for r in all_redis + [nc, nc_badpass, nc_nopass]:
        r.clean()
        r.deploy()
        r.stop()
        r.start()

def teardown():
    for r in all_redis + [nc, nc_badpass, nc_nopass]:
        assert(r._alive())
        r.stop()

default_kv = {'kkk-%s' % i : 'vvv-%s' % i for i in range(10)}

def getconn():
    r = redis.Redis(nc.host(), nc.port())
    return r

'''

cases:


redis       proxy       case
1           1           test_auth_basic
1           bad         test_badpass_on_proxy
1           0           test_nopass_on_proxy
0           0           already tested on other case
0           1

'''

def test_auth_basic():
    # we hope to have same behavior when the server is redis or twemproxy
    conns = [
         redis.Redis(all_redis[0].host(), all_redis[0].port()),
         redis.Redis(nc.host(), nc.port()),
    ]

    for r in conns:
        assert_fail('NOAUTH|operation not permitted', r.ping)
        assert_fail('NOAUTH|operation not permitted', r.set, 'k', 'v')
        assert_fail('NOAUTH|operation not permitted', r.get, 'k')

        # bad passwd
        assert_fail('invalid password', r.execute_command, 'AUTH', 'badpasswd')

        # everything is ok after auth
        r.execute_command('AUTH', 'hellopasswd')
        r.set('k', 'v')
        assert(r.ping() == True)
        assert(r.get('k') == 'v')

        # auth fail here, should we return ok or not => we will mark the conn state as not authed
        assert_fail('invalid password', r.execute_command, 'AUTH', 'badpasswd')

        assert_fail('NOAUTH|operation not permitted', r.ping)
        assert_fail('NOAUTH|operation not permitted', r.get, 'k')

def test_nopass_on_proxy():
    r = redis.Redis(nc_nopass.host(), nc_nopass.port())

    # if you config pass on redis but not on twemproxy,
    # twemproxy will reply ok for ping, but once you do get/set, you will get errmsg from redis
    assert(r.ping() == True)
    assert_fail('NOAUTH|operation not permitted', r.set, 'k', 'v')
    assert_fail('NOAUTH|operation not permitted', r.get, 'k')

    # proxy has no pass, when we try to auth
    assert_fail('Client sent AUTH, but no password is set', r.execute_command, 'AUTH', 'anypasswd')
    pass

def test_badpass_on_proxy():
    r = redis.Redis(nc_badpass.host(), nc_badpass.port())

    assert_fail('NOAUTH|operation not permitted', r.ping)
    assert_fail('NOAUTH|operation not permitted', r.set, 'k', 'v')
    assert_fail('NOAUTH|operation not permitted', r.get, 'k')

    # we can auth with bad pass (twemproxy will say ok for this)
    r.execute_command('AUTH', 'badpasswd')
    # after that, we still got NOAUTH for get/set (return from redis-server)
    assert(r.ping() == True)
    assert_fail('NOAUTH|operation not permitted', r.set, 'k', 'v')
    assert_fail('NOAUTH|operation not permitted', r.get, 'k')

def setup_and_wait():
    time.sleep(60*60)
