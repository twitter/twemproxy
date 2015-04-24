#!/usr/bin/env python
#coding: utf-8

from common import *

def test_linsert():
    r = getconn()

    r.rpush('mylist', 'Hello')
    r.rpush('mylist', 'World')
    r.linsert('mylist', 'BEFORE', 'World', 'There')

    rst = r.lrange('mylist', 0, -1)
    assert(rst == ['Hello', 'There', 'World'])

def test_lpush_lrange():
    r = getconn()

    vals = ['vvv-%s' % i for i in range(10) ]
    assert([] == r.lrange('mylist', 0, -1))

    r.lpush('mylist', *vals)
    rst = r.lrange('mylist', 0, -1)

    assert(10 == len(rst))

def test_hscan():
    r = getconn()

    kv = {'kkk-%s' % i : 'vvv-%s' % i for i in range(10)}
    r.hmset('a', kv)

    cursor, dic = r.hscan('a')
    assert(str(cursor) == '0')
    assert(dic == kv)

    cursor, dic = r.hscan('a', match='kkk-5')
    assert(str(cursor) == '0')
    assert(dic == {'kkk-5': 'vvv-5'})

def test_hscan_large():
    r = getconn()

    kv = {'x'* 100 + 'kkk-%s' % i : 'vvv-%s' % i for i in range(1000)}
    r.hmset('a', kv)

    cursor = '0'
    dic = {}
    while True:
        cursor, t = r.hscan('a', cursor, count=10)
        for k, v in t.items():
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

    r.zadd('a', 'a', 1, 'b', 2, 'c', 3)

    cursor, pairs = r.zscan('a')
    assert(str(cursor) == '0')
    assert(set(pairs) == set([('a', 1), ('b', 2), ('c', 3)]))

    cursor, pairs = r.zscan('a', match='a')
    assert(str(cursor) == '0')
    assert(set(pairs) == set([('a', 1)]))

def test_sscan():
    r = getconn()

    r.sadd('a', 1, 2, 3)

    cursor, members = r.sscan('a')
    assert(str(cursor) == '0')
    assert(set(members) == set(['1', '2', '3']))

    cursor, members = r.sscan('a', match='1')
    assert(str(cursor) == '0')
    assert(set(members) == set(['1']))

