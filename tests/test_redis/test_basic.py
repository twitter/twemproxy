#!/usr/bin/env python
#coding: utf-8

from nose.tools import nottest
from .common import *

def test_setget():
    r = getconn()

    rst = r.set('k', 'v')
    assert_equal(True, rst)
    assert_equal(b'v', r.get('k'))

def test_msetnx():
    r = getconn()

    # https://redis.io/commands/msetnx
    # MSETNX is not supported because the keys can get sent to different backends, which is not supported.
    normalized_kv = {str(key, encoding='utf-8'): val for key, val in default_kv.items()}
    assert_fail('Socket closed|Connection closed', r.msetnx, normalized_kv)

def test_null_key():
    r = getconn()
    rst = r.set('', 'v')
    assert_equal(b'v', r.get(''))

    rst = r.set('', '')
    assert_equal(b'', r.get(''))

    kv = {'' : 'val', 'k': 'v'}
    ret = r.mset(kv)
    assert_equal(b'val', r.get(''))

def test_ping_quit():
    r = getconn()
    assert(r.ping() == True)

    #get set
    rst = r.set('k', 'v')
    assert_equal(b'v', r.get('k'))

    assert_fail('Socket closed|Connection closed', r.execute_command, 'QUIT')

def test_slow_req_lua():
    r = getconn()
    pipe = r.pipeline(transaction=False)
    pipe.eval("local x=0;for i = 1,300000000,1 do x = x+ i; end; return x", 1, 'tmp')
    assert_fail('timed out', pipe.execute)

def test_fast_req_lua():
    r = getconn()
    pipe = r.pipeline(transaction=False)
    pipe.eval("local x=0;for i = 1,100,1 do x = x+ i; end; return x", 1, 'tmp')
    assert_equal([5050], pipe.execute())

# Disabled because this uses a lot of memory and would sometimes complete before the timeout.
@nottest
def disabled_test_slow_req():
    r = getconn()

    kv = {'mkkk-%s' % i : 'mvvv-%s' % i for i in range(500000)}

    pipe = r.pipeline(transaction=False)
    pipe.set('key-1', 'v1')
    pipe.get('key-1')
    pipe.hmset('xxx', kv)
    pipe.get('key-2')
    pipe.get('key-3')

    assert_fail('timed out', pipe.execute)

def test_signal():
    #init
    nc.cleanlog()
    time.sleep(.1)
    nc.signal('HUP')

    nc.signal('HUP')
    nc.signal('TTIN')
    nc.signal('TTOU')
    nc.signal('SEGV')

    time.sleep(.3)
    log = open(nc.logfile(), 'r').read()

    assert(strstr(log, 'HUP'))
    assert(strstr(log, 'TTIN'))
    assert(strstr(log, 'TTOU'))
    assert(strstr(log, 'SEGV'))

    #recover
    nc.start()

def test_nc_stats():
    nc.stop() #reset counters
    nc.start()
    r = getconn()
    kv = {'kkk-%s' % i :'vvv-%s' % i for i in range(10)}
    for k, v in list(kv.items()):
        r.set(k, v)
        r.get(k)

    def get_stat(name):
        time.sleep(1)
        stat = nc._info_dict()
        #pprint(stat)
        if name in ['client_connections', 'client_eof', 'client_err', \
                    'forward_error', 'fragments', 'server_ejects']:
            return stat[CLUSTER_NAME][name]

        #sum num of each server
        ret = 0
        for k, v in list(stat[CLUSTER_NAME].items()):
            if type(v) == dict:
                ret += v[name]
        return ret

    assert(get_stat('requests') == 20)
    assert(get_stat('responses') == 20)

    ##### mget
    keys = list(kv.keys())
    r.mget(keys)

    #for version<=0.3.0
    #assert(get_stat('requests') == 30)
    #assert(get_stat('responses') == 30)

    #for mget-improve
    assert(get_stat('requests') == 22)
    assert(get_stat('responses') == 22)

def test_issue_323():
    # do on redis
    r = all_redis[0]
    c = redis.Redis(r.host(), r.port())
    assert_equal([1, b'OK'], c.eval("return {1, redis.call('set', 'x', '1')}", 1, 'tmp'))

    # do on twemproxy
    c = getconn()
    assert_equal([1, b'OK'], c.eval("return {1, redis.call('set', 'x', '1')}", 1, 'tmp'))

    # Test processing deeply nested multibulk responses
    assert_equal([[[[[[[[[[[[[[[[[[[[b'value']]]]]]]]]]]]]]]]]]], b'other'], c.eval("return {{{{{{{{{{{{{{{{{{{{'value'}}}}}}}}}}}}}}}}}}}, 'other'}", 1, 'tmp'))

def setup_and_wait():
    time.sleep(60*60)

