#!/usr/bin/env python
#coding: utf-8

from .common import *

def test_linsert():
    r = getconn()

    r.rpush('mylist', 'Hello')
    r.rpush('mylist', 'World')
    r.linsert('mylist', 'BEFORE', 'World', 'There')

    rst = r.lrange('mylist', 0, -1)
    assert_equal([b'Hello', b'There', b'World'], rst)

def test_exists():
    r = getconn()

    r.set('exists1', 'foo')
    assert_equal(1, r.exists('exists1'))
    assert_equal(0, r.exists('doesnotexist'))

def test_lpush_lrange():
    r = getconn()

    vals = [b'vvv-%d' % i for i in range(10) ]
    assert([] == r.lrange('mylist', 0, -1))

    r.lpush('mylist', *vals)
    rst = r.lrange('mylist', 0, -1)

    assert(10 == len(rst))

def test_hscan():
    r = getconn()

    kv = {b'kkk-%d' % i : b'vvv-%d' % i for i in range(10)}
    r.hmset('a', kv)

    cursor, dic = r.hscan('a')
    assert(str(cursor) == '0')
    assert(dic == kv)

    cursor, dic = r.hscan('a', match='kkk-5')
    assert(str(cursor) == '0')
    assert(dic == {b'kkk-5': b'vvv-5'})

def test_hscan_large():
    r = getconn()

    kv = {b'x'* 100 + (b'kkk-%d' % i) : (b'vvv-%d' % i)
          for i in range(1000)}
    r.hmset('a', kv)

    cursor = '0'
    dic = {}
    while True:
        cursor, t = r.hscan('a', cursor, count=10)
        for k, v in list(t.items()):
            dic[k] = v

        if '0' == str(cursor):
            break

    assert(dic == kv)

    cursor, dic = r.hscan('a', '0', match='*kkk-5*', count=1000)
    if str(cursor) == '0':
        assert(len(dic) == 111)
    else:
        assert(len(dic) == 111)

        #again.
        cursor, dic = r.hscan('a', cursor, match='*kkk-5*', count=1000)
        assert(str(cursor) == '0')
        assert(len(dic) == 0)

def test_zscan():
    r = getconn()

    r.zadd('a', {'a': 1, 'b': 2, 'c': 3})

    cursor, pairs = r.zscan('a')
    assert_equal(0, cursor)
    assert_equal({(b'a', 1), (b'b', 2), (b'c', 3)}, set(pairs))

    cursor, pairs = r.zscan('a', match='a')
    assert_equal(0, cursor)
    assert_equal({(b'a', 1)}, set(pairs))

def test_sscan():
    r = getconn()

    r.sadd('a', 1, 2, 3)

    cursor, members = r.sscan('a')
    assert_equal(0, cursor)
    assert_equal({b'1', b'2', b'3'}, set(members))

    cursor, members = r.sscan('a', match='1')
    assert_equal('0', str(cursor))
    assert_equal({b'1'}, set(members))


def test_scan():
    r = getconn()
    r.set('hello_scan_a1',11)
    r.set('hello_scan_b1',22)
    r.hmset("hello_scan_h1",{"a":1,"b":2})
    r.rpush("hello_scan_l1","a","b")
    r.sadd("hello_scan_s1","a","a","b")
    r.zadd("hello_scan_z1",{"one": 1, "two": 2, "three": 3})

    zsetval=r.zrange("hello_scan_z1",0,-1,False,True)

    cursor = 0
    match_str = "hello_scan_*"
    rets = []
    subrets = []
    while True:
        cursor,subrets =  r.scan(cursor,match_str,100)
        if len(subrets):
            rets.extend(subrets)
        if cursor == 0:
            break
    rets.sort()
    assert_equal(rets,[b'hello_scan_a1', b'hello_scan_b1', b'hello_scan_h1', b'hello_scan_l1', b'hello_scan_s1', b'hello_scan_z1'])

def test_script_load_and_exits():
    r = getconn()

    evalsha=r.script_load("return redis.call('hset',KEYS[1],KEYS[1],KEYS[1])")
    assert_equal(evalsha,"dbbae75a09f1390aaf069fb60e951ec23cab7a15")

    exists=r.script_exists("dbbae75a09f1390aaf069fb60e951ec23cab7a15")
    assert_equal([True],exists)

    assert_equal(1,r.evalsha("dbbae75a09f1390aaf069fb60e951ec23cab7a15",1,"scriptA"))

    dic=r.hgetall("scriptA")
    assert_equal(dic,{b'scriptA': b'scriptA'})

    assert_equal(True,r.script_flush())

def test_select():
    r = getconn()
    val=r.select(0)
    assert_equal(True,val)

    assert_fail("only select 0 support",r.select,1)
