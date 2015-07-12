#!/usr/bin/env python
#coding: utf-8

from common import *
from test_mget_mset import test_mget_mset as _mget_mset

#force to use large mbuf, we need to copy the setup/teardown here..

mbuf = 64*1024

nc = NutCracker(nc.host(), nc.port(), '/tmp/r/nutcracker-4100', CLUSTER_NAME,
                all_redis, mbuf=mbuf, verbose=nc_verbose)

def setup():
    print 'special setup(mbuf=%s, verbose=%s)' %(mbuf, nc_verbose)
    for r in all_redis + [nc]:
        r.deploy()
        r.stop()
        r.start()

def teardown():
    for r in all_redis + [nc]:
        assert(r._alive())
        r.stop()

######################################################
def test_mget_binary_value(cnt=5):
    kv = {}
    for i in range(cnt):
        kv['kkk-%s' % i] = os.urandom(1024*1024*16+1024) #16M
    for i in range(cnt):
        kv['kkk2-%s' % i] = ''
    _mget_mset(kv)

