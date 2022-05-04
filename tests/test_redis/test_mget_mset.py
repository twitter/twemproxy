#!/usr/bin/env python3

from .common import *

def test_mget_mset(kv=default_kv):
    r = getconn()

    def insert_by_pipeline():
        pipe = r.pipeline(transaction=False)
        for k, v in list(kv.items()):
            pipe.set(k, v)
        pipe.execute()

    def insert_by_mset():
        ret = r.mset(**kv)

    #insert_by_mset() #only the mget-imporve branch support this
    try:
        insert_by_mset() #only the mget-imporve branch support this
    except:
        insert_by_pipeline()

    keys = list(kv.keys())

    #mget to check
    vals = r.mget(keys)
    for i, k in enumerate(keys):
        assert_equal(kv[k], vals[i])

    #del
    assert (len(keys) == r.delete(*keys) )

    #mget again
    vals = r.mget(keys)

    for i, k in enumerate(keys):
        assert(None == vals[i])

def test_mget_mset_on_key_not_exist(kv=default_kv):
    r = getconn()

    def insert_by_pipeline():
        pipe = r.pipeline(transaction=False)
        for k, v in list(kv.items()):
            pipe.set(k, v)
        pipe.execute()

    def insert_by_mset():
        ret = r.mset(**kv)

    try:
        insert_by_mset() #only the mget-imporve branch support this
    except:
        insert_by_pipeline()

    keys = list(kv.keys())
    keys2 = [b'x-'+k for k in keys]
    keys = keys + keys2
    random.shuffle(keys)

    #mget to check
    vals = r.mget(keys)
    for i, k in enumerate(keys):
        if k in kv:
            assert_equal(kv[k], vals[i])
        else:
            assert_equal(None, vals[i])

    #del
    assert (len(kv) == r.delete(*keys) )

    #mget again
    vals = r.mget(keys)

    for i, k in enumerate(keys):
        assert(None == vals[i])

def test_mget_mset_large():
    for cnt in range(171, large, 171):
        kv = {b'kkk-%d' % i : b'vvv-%d' % i for i in range(cnt)}
        test_mget_mset(kv)

def test_mget_special_key(cnt=5):
    #key length = 512-48-1
    kv = {}
    for i in range(cnt):
        k = b'kkk-%d' % i
        k = k + b'x'*(512-48-1-len(k))
        kv[k] = b'vvv'

    test_mget_mset(kv)

def test_mget_special_key_2(cnt=5):
    #key length = 512-48-2
    kv = {}
    for i in range(cnt):
        k = b'kkk-%d' % i
        k = k + b'x'*(512-48-2-len(k))
        kv[k] = b'vvv'*9

    test_mget_mset(kv)

def test_mget_on_backend_down():
    #one backend down

    r = redis.Redis(nc.host(), nc.port())
    assert_equal(None, r.get('key-2'))
    assert_equal(None, r.get('key-1'))

    all_redis[0].stop()

    assert_fail('Connection refused|reset by peer|Broken pipe', r.mget, 'key-1')
    assert_fail('Connection refused|reset by peer|Broken pipe', r.get, 'key-1')
    assert_equal(None, r.get('key-2'))

    keys = ['key-1', 'key-2', 'kkk-3']
    assert_fail('Connection refused|reset by peer|Broken pipe', r.mget, *keys)

    #all backend down
    all_redis[1].stop()
    r = redis.Redis(nc.host(), nc.port())

    assert_fail('Connection refused|reset by peer|Broken pipe', r.mget, 'key-1')
    assert_fail('Connection refused|reset by peer|Broken pipe', r.mget, 'key-2')

    keys = ['key-1', 'key-2', 'kkk-3']
    assert_fail('Connection refused|reset by peer|Broken pipe', r.mget, *keys)

    for r in all_redis:
        r.start()

def test_mset_on_backend_down():
    all_redis[0].stop()
    r = redis.Redis(nc.host(),nc.port())

    assert_fail('Connection refused|Broken pipe',r.mset,default_kv)

    all_redis[1].stop()
    assert_fail('Connection refused|Broken pipe',r.mset,default_kv)

    for r in all_redis:
        r.start()

def test_mget_pipeline():
    r = getconn()

    pipe = r.pipeline(transaction=False)
    for k,v in list(default_kv.items()):
        pipe.set(k,v)
    keys = list(default_kv.keys())
    pipe.mget(keys)
    kv = {}
    for i in range(large):
        kv[b'kkk-%d' % i] = os.urandom(100)
    for k,v in list(kv.items()):
        pipe.set(k,v)
    for k in list(kv.keys()):
        pipe.get(k)
    rst = pipe.execute()

    #print rst
    #check the result
    keys = list(default_kv.keys())

    #mget to check
    vals = r.mget(keys)
    for i, k in enumerate(keys):
        assert(kv[k] == vals[i])

    #del
    assert (len(keys) == r.delete(*keys) )

    #mget again
    vals = r.mget(keys)

    for i, k in enumerate(keys):
        assert(None == vals[i])

def test_multi_delete_normal():
    r = getconn()

    for i in range(100):
        r.set('key-%s'%i, 'val-%s'%i)

    for i in range(100):
        assert_equal(bytes('val-%s'%i, encoding='utf-8'), r.get('key-%s'%i) )

    keys = ['key-%s'%i for i in range(100)]
    assert_equal(100, r.delete(*keys))

    for i in range(100):
        assert_equal(None, r.get('key-%s'%i) )

def test_multi_delete_on_readonly():
    all_redis[0].slaveof(all_redis[1].args['host'], all_redis[1].args['port'])

    r = redis.Redis(nc.host(), nc.port())

    # got "You can't write against a read only (replica)"
    assert_fail('READONLY|Invalid|You can\'t write against a read only', r.delete, 'key-1')
    assert_equal(0, r.delete('key-2'))
    assert_fail('READONLY|Invalid|You can\'t write against a read only', r.delete, 'key-3')

    keys = ['key-1', 'key-2', 'kkk-3']
    assert_fail('Invalid argument', r.delete, *keys)     # got "Invalid argument"

def test_multi_delete_on_backend_down():
    r = redis.Redis(nc.host(), nc.port())
    assert_equal(None, r.get('key-2'))

    # one backend down
    all_redis[0].stop()

    try:
        # Saw this fail in redis 6.2.2 spuriously in GitHub actions with a timeout.
        # Continue to assert that subsequent commands will recover.
        assert_fail('Connection refused|reset by peer|Broken pipe|Connection timed out', r.delete, 'key-1')
        assert_equal(None, r.get('key-2'))

        keys = ['key-1', 'key-2', 'kkk-3']
        assert_fail('Connection refused|reset by peer|Broken pipe', r.delete, *keys)

        #all backend down
        all_redis[1].stop()
        r = redis.Redis(nc.host(), nc.port())

        assert_fail('Connection refused|reset by peer|Broken pipe', r.delete, 'key-1')
        assert_fail('Connection refused|reset by peer|Broken pipe', r.delete, 'key-2')

        keys = ['key-1', 'key-2', 'kkk-3']
        assert_fail('Connection refused|reset by peer|Broken pipe', r.delete, *keys)
    finally:
        # Start is idempotent.
        for r in all_redis:
            r.start()


def test_multi_delete_20140525():
    r = getconn()

    cnt = 126
    keys = ['key-%s'%i for i in range(cnt)]
    pipe = r.pipeline(transaction=False)
    pipe.mget(keys)
    pipe.delete(*keys)
    pipe.execute()


