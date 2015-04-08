# Check https://github.com/tarantool/tarantool-python for installation
# instructions

import tarantool
import os
import time

range=10

def verify(space, results, server):
    print server.center(30, '*')
    for x in results:
        key = x[0]
        val = x[1]
        res = space.select(key)
        print res

        assert res.rowcount == 1, 'Cannot find key ' + key
        assert res[0][0] == key and res[0][1] == val, \
        'Unexpected response from ' + server + ': ' + str(res)

def verify_call(conn, procname, results, server):
    print server.center(30, '*')
    for x in results:
        key = x[0]
        val = x[1]
        res = conn.call(procname, key)
        print res

        assert res.rowcount == 1, 'Cannot find key ' + key
        assert res[0][0] == key and res[0][1] == val, \
        'Unexpected response from ' + server + ': ' + str(res)

os.system("./tarantool-setup.sh")

time.sleep(5)

proxy = tarantool.connect(host = 'localhost', port=22121)
ptest = proxy.space('test')

con1 = tarantool.connect(host = 'localhost', port=3301)
c1test = con1.space('test')

con2 = tarantool.connect(host = 'localhost', port=3302)
c2test = con2.space('test')

con3 = tarantool.connect(host = 'localhost', port=3303)
c3test = con3.space('test')

print '=============================='
print '          ping tests          '
print '=============================='

proxy.ping()

print '=============================='
print '          auth tests          '
print '=============================='

proxy.authenticate('test', 'test');

print '=============================='
print '        insert tests          '
print '=============================='

for x in xrange(range):
    ptest.insert(('key' + str(x), x))

verify(c1test, [['key2', 2], ['key7', 7], ['key8', 8]], "Server 1")

verify(c2test, [['key1', 1], ['key3', 3], ['key6', 6], ['key9', 9]], "Server 2")

verify(c3test, [['key0', 0], ['key4', 4], ['key5', 5]], "Server 3")

print '=============================='
print '         select tests         '
print '=============================='

verify(ptest, [['key0', 0], ['key1', 1], ['key2', 2], ['key3', 3], ['key4', 4],
               ['key5', 5], ['key6', 6], ['key7', 7], ['key8', 8], ['key9', 9]],
       "twemproxy")

print '=============================='
print '        replace tests         '
print '=============================='

for x in xrange(range):
    ptest.replace(('key' + str(x), -x))

verify(c1test, [['key2', -2], ['key7', -7], ['key8', -8]], "Server 1")

verify(c2test, [['key1', -1], ['key3', -3], ['key6', -6], ['key9', -9]],
       "Server 2")

verify(c3test, [['key0', 0], ['key4', -4], ['key5', -5]], "Server 3")

print '=============================='
print '        update tests          '
print '=============================='

for x in xrange(range):
    ptest.update('key' + str(x), [('#', 1, 1)])

for x in xrange(range):
    ptest.update('key' + str(x), [('!', 1, x)])

verify(ptest, [['key0', 0], ['key1', 1], ['key2', 2], ['key3', 3], ['key4', 4],
               ['key5', 5], ['key6', 6], ['key7', 7], ['key8', 8], ['key9', 9]],
       "twemproxy")

verify(c1test, [['key2', 2], ['key7', 7], ['key8', 8]], "Server 1")

verify(c2test, [['key1', 1], ['key3', 3], ['key6', 6], ['key9', 9]], "Server 2")

verify(c3test, [['key0', 0], ['key4', 4], ['key5', 5]], "Server 3")

print '=============================='
print '          call tests          '
print '=============================='

verify_call(proxy, 'box.space.test:select',
            [['key0', 0], ['key1', 1], ['key2', 2], ['key3', 3], ['key4', 4],
             ['key5', 5], ['key6', 6], ['key7', 7], ['key8', 8], ['key9', 9]],
            "twemproxy")
