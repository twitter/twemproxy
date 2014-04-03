#!/usr/bin/env python
#coding: utf-8
#file   : test_mget.py
#author : ning
#date   : 2014-04-01 13:15:48

'''
usage:

export REDIS_HOST=127.0.0.1
export REDIS_PORT=4000
nosetests

'''

import os
import redis

host = os.environ['REDIS_HOST']
port = int(os.environ['REDIS_PORT'])


def test_mget(cnt=10):
    print 'test_many', cnt
    r = redis.StrictRedis(host, port)

    def insert_by_pipeline():
        pipe = r.pipeline(transaction=False)
        for i in range(cnt):
            pipe.set('kkk-%s'%i, 'vvv-%s'%i)
        pipe.execute()

    def insert_by_mset():
        kv = {'kkk-%s' % i :'vvv-%s' % i for i in range(cnt)}
        ret = r.mset(**kv)

    insert_by_mset()
    keys = ['kkk-%s' % i for i in range(cnt)]

    #mget to check
    vals = r.mget(keys)
    #print 'vals', vals
    for i in range(cnt):
        assert('vvv-%s'%i == vals[i])

    #del
    assert (cnt == r.delete(*keys) )

    #mget again
    vals = r.mget(keys)
    for i in range(cnt):
        assert(None == vals[i])

def test_many_mget():
    for i in range(1, 10000, 17):
        test_mget(i)
    pass

def test_large_mget(cnt=5):
    r = redis.StrictRedis(host, port)

    kv = {}
    for i in range(cnt):
        kv['kkx-%s' % i] = os.urandom(1024*1024*8)

    #insert
    for i in range(cnt):
        key = 'kkx-%s' % i
        r.set(key, kv[key])

    keys = ['kkx-%s' % i for i in range(cnt)]

    #mget to check
    vals = r.mget(keys)
    for i in range(cnt):
        key = 'kkx-%s' % i
        assert(kv[key] == vals[i])


