#!/usr/bin/env python3

from .common import *

def test_pipeline():
    r = getconn()

    pipe = r.pipeline(transaction = False)

    pipe.set('a', 'a1').get('a').zadd('z', {'z1': 1}).zadd('z', {'z2': 4})
    pipe.zincrby('z', 1, 'z1').zrange('z', 0, 5, withscores=True)

    assert_equal(
        [
            True,
            b'a1',
            True,
            True,
            2.0,
            [(b'z1', 2.0), (b'z2', 4)],
        ],
        pipe.execute()
    )

def test_invalid_pipeline():
    r = getconn()

    pipe = r.pipeline(transaction = False)

    pipe.set('a', 1).set('b', 2).lpush('a', 3).set('d', 4).get('a')
    result = pipe.execute(raise_on_error = False)

    assert_equal(True, result[0])
    assert_equal(True, result[1])

    # we can't lpush to a key that's a string value, so this should
    # be a ResponseError exception
    assert isinstance(result[2], redis.ResponseError)

    # since this isn't a transaction, the other commands after the
    # error are still executed
    assert_equal(True, result[3])
    assert_equal(b'1', result[4])

    # make sure the pipe was restored to a working state
    assert pipe.set('z', 'zzz').execute() == [True]

def test_parse_error_raised():
    r = getconn()

    pipe = r.pipeline(transaction = False)

    # the zrem is invalid because we don't pass any keys to it
    pipe.set('a', 1).zrem('b').set('b', 2)
    result = pipe.execute(raise_on_error = False)

    assert result[0]
    assert isinstance(result[1], redis.ResponseError)
    assert result[2]
