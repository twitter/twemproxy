Python testing facilities for twemproxy, this test suite is based on https://github.com/idning/redis-mgr

Testing in docker
=================

The script `test_in_docker.sh` can be run to run all of twemproxy's compiler checks, C unit tests, and this folder's integration tests in docker.

    REDIS_VERSION=6.2.4
    ./test_in_docker.sh $REDIS_VERSION

usage
=====

Information on setting up integration tests, running integration tests, and creating new integration tests is below.

1. install dependencies (redis-py must be 3.0 or newer)::

    pip install nose
    pip install git+https://github.com/andymccurdy/redis-py.git@3.5.3
    pip install git+https://github.com/linsomniac/python-memcached.git#egg=memcache

2. copy binaries to _binaries/::

    _binaries/
    |-- nutcracker
    |-- redis-benchmark
    |-- redis-check-aof
    |-- redis-check-dump
    |-- redis-cli
    |-- redis-sentinel
    |-- redis-server
    |-- memcached

3. run with nosetests (or ./nosetests_verbose.sh)::

    $ python3 -m nose -v
    test_del.test_multi_delete_on_readonly ... ok
    test_mget.test_mget ... ok

    ----------------------------------------------------------------------
    Ran 2 tests in 4.483s

    OK

4. add a case::

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

- After all of the integration tests, you may get a core dump because we have a case in test_signal which will send SEGV to nutcracker

- If tests are failing, you may have to `pkill` redis-server, redis-sentinel, or nutcracker

Unit tests
==========

See src/test_all.c - unit tests are separate from these integration tests and do not require python. They can be compiled and run with `make check`.

To view the output of the failing tests, run `cd src; make test_all; ./test_all`.
