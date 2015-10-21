Python testing facilities for twemproxy, this test suite is based on https://github.com/idning/redis-mgr

already add to https://travis-ci.org/idning/twemproxy as travis-ci

see https://github.com/idning/twemproxy/blob/travis-ci/travis.sh

usage
=====

1. install dependency::

    pip install nose
    pip install git+https://github.com/andymccurdy/redis-py.git@2.10.3
    pip install git+https://github.com/idning/python-memcached.git#egg=memcache

2. copy binarys to _binaries/::

    _binaries/
    |-- nutcracker
    |-- redis-benchmark
    |-- redis-check-aof
    |-- redis-check-dump
    |-- redis-cli
    |-- redis-sentinel
    |-- redis-server
    |-- memcached

3. run::

    $ nosetests -v
    test_del.test_multi_delete_on_readonly ... ok
    test_mget.test_mget ... ok

    ----------------------------------------------------------------------
    Ran 2 tests in 4.483s

    OK

4. add A case::

    cp tests/test_del.py tests/test_xxx.py
    vim tests/test_xxx.py



variables
=========
::

    export T_VERBOSE=9 will start nutcracker with '-v 9'  (default:4)
    export T_MBUF=512  will start nutcracker whit '-m 512' (default:521)
    export T_LARGE=10000 will test 10000 keys for mget/mset (default:1000)

T_LOGFILE:

- to put test log on stderr::

    export T_LOGFILE=-

- to put test log on t.log::

    export T_LOGFILE=t.log

  or::

    unset T_LOGFILE


notes
=====

- After all the tests. you may got a core because we have a case in test_signal which will send SEGV to nutcracker


