import redis

range=100
factor=32
port=22121

r = redis.StrictRedis(host='localhost', port=port, db=0)

# lrange
print [r.lrange('lfoo', 0, x) for x in xrange(1, range)]
print [r.lpush('lfoo', str(x)*factor) for x in xrange(1, range)]
print [r.lrange('lfoo', 0, x) for x in xrange(1, range)]
print r.delete('lfoo')

# del
print [r.set('foo' + str(x), str(x)*factor) for x in xrange(1, range)]
keys = ['foo' + str(x) for x in xrange(1, range)]
print [r.delete(keys) for x in xrange(1, range)]

# mget
print [r.set('foo' + str(x), str(x)*100) for x in xrange(1, range)]
keys = ['foo' + str(x) for x in xrange(1, range)]
print [r.mget(keys) for x in xrange(1, range)]
