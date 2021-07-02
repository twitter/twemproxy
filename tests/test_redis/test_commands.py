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

