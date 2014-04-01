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


def test_mget(cnt=1000):
    print 'test_many', cnt
    r = redis.StrictRedis(host, port)

    #insert
    pipe = r.pipeline(transaction=False)
    for i in range(cnt):
        pipe.set('kkk-%s'%i, 'vvv-%s'%i)
    pipe.execute()

    keys = ['kkk-%s' % i for i in range(cnt)]

    #mget to check
    vals = r.mget(keys)
    #print vals
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


