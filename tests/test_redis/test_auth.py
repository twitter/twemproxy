#!/usr/bin/env python3

from .common import *

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
    print('setup(mbuf=%s, verbose=%s)' %(mbuf, nc_verbose))
    for r in all_redis + [nc, nc_badpass, nc_nopass]:
        r.clean()
        r.deploy()
        r.stop()
        r.start()

def teardown():
    for r in all_redis + [nc, nc_badpass, nc_nopass]:
        assert(r._alive())
        r.stop()

default_kv = {bytes('kkk-%s' % i, encoding='utf-8') : bytes('vvv-%s' % i, encoding='utf-8') for i in range(10)}

def getconn():
    r = redis.Redis(nc.host(), nc.port(), decode_responses=True)
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
         redis.Redis(all_redis[0].host(), all_redis[0].port(), decode_responses=True),
         redis.Redis(nc.host(), nc.port(), decode_responses=True),
    ]

    for r in conns:
        assert_fail('Authentication required', r.ping)
        assert_fail('Authentication required', r.set, 'k', 'v')
        assert_fail('Authentication required', r.get, 'k')

        # bad passwd
        assert_fail('invalid password|WRONGPASS', r.execute_command, 'AUTH', 'badpasswd')

        # everything is ok after auth
        r.execute_command('AUTH', 'hellopasswd')
        r.set('k', 'v')
        assert(r.ping() == True)
        assert_equal(b'v', r.get('k'))

        # auth fail here, should we return ok or not => we will mark the conn state as not authed
        assert_fail('invalid password|WRONGPASS', r.execute_command, 'AUTH', 'badpasswd')
        # https://redis.io/commands/auth changed in redis 6.0.0 and auth now appears to be additive for valid credentials?
        # We can get the redis version by invoking a shell command, but not going to bother. Just assert that it if it throws, it's for the expected reason.
        try:
            r.ping()
        except Exception as e:
            assert re.search('Authentication required', str(e))
        try:
            r.get('k')
        except Exception as e:
            assert re.search('Authentication required', str(e))

def test_nopass_on_proxy():
    r = redis.Redis(nc_nopass.host(), nc_nopass.port(), decode_responses=True)

    # if you config pass on redis but not on twemproxy,
    # twemproxy will reply ok for ping, but once you do get/set, you will get errmsg from redis
    assert(r.ping() == True)
    assert_fail('Authentication required', r.set, 'k', 'v')
    assert_fail('Authentication required', r.get, 'k')

    # proxy has no pass, when we try to auth
    assert_fail('Client sent AUTH, but no password is set', r.execute_command, 'AUTH', 'anypasswd')
    pass

def test_badpass_on_proxy():
    r = redis.Redis(nc_badpass.host(), nc_badpass.port(), decode_responses=True)

    assert_fail('Authentication required', r.ping)
    assert_fail('Authentication required', r.set, 'k', 'v')
    assert_fail('Authentication required', r.get, 'k')

    # we can auth with bad pass (twemproxy will say ok for this)
    r.execute_command('AUTH', 'badpasswd')
    # after that, we still got NOAUTH for get/set (return from redis-server)
    assert(r.ping() == True)
    assert_fail('Authentication required', r.set, 'k', 'v')
    assert_fail('Authentication required', r.get, 'k')

def setup_and_wait():
    time.sleep(60*60)
