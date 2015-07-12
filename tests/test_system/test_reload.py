#!/usr/bin/env python
#coding: utf-8
#file   : test_reload.py
#author : ning
#date   : 2014-09-03 12:28:16

import os
import sys
import redis

PWD = os.path.dirname(os.path.realpath(__file__))
WORKDIR = os.path.join(PWD,'../')
sys.path.append(os.path.join(WORKDIR,'lib/'))
sys.path.append(os.path.join(WORKDIR,'conf/'))

import conf

from server_modules import *
from utils import *
from nose import with_setup

CLUSTER_NAME = 'ntest'
nc_verbose = int(getenv('T_VERBOSE', 5))
mbuf = int(getenv('T_MBUF', 512))
large = int(getenv('T_LARGE', 1000))

T_RELOAD_DELAY = 3 + 1

all_redis = [
        RedisServer('127.0.0.1', 2100, '/tmp/r/redis-2100/', CLUSTER_NAME, 'redis-2100'),
        RedisServer('127.0.0.1', 2101, '/tmp/r/redis-2101/', CLUSTER_NAME, 'redis-2101'),
    ]

nc = NutCracker('127.0.0.1', 4100, '/tmp/r/nutcracker-4100', CLUSTER_NAME,
                all_redis, mbuf=mbuf, verbose=nc_verbose)

def _setup():
    print 'setup(mbuf=%s, verbose=%s)' %(mbuf, nc_verbose)
    for r in all_redis + [nc]:
        r.deploy()
        r.stop()
        r.start()

def _teardown():
    for r in all_redis + [nc]:
        assert(r._alive())
        r.stop()

def get_tcp_conn(host, port):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((host, port))
    s.settimeout(.3)
    return s

def send_cmd(s, req, resp):
    s.sendall(req)
    data = s.recv(10000)
    assert(data == resp)

@with_setup(_setup, _teardown)
def test_reload_with_old_conf():
    if nc.version() < '0.4.2':
        print 'Ignore test_reload for version %s' % nc.version()
        return
    pid = nc.pid()
    # print 'old pid:', pid
    r = redis.Redis(nc.host(), nc.port())
    r.set('k', 'v')

    conn = get_tcp_conn(nc.host(), nc.port())
    send_cmd(conn, '*2\r\n$3\r\nGET\r\n$1\r\nk\r\n', '$1\r\nv\r\n')

    # nc.reload() is same as nc.stop() and nc.start()
    nc.reload()
    time.sleep(.01)  #it need time for the old process fork new process.

    # the old connection is still ok in T_RELOAD_DELAY seconds
    send_cmd(conn, '*2\r\n$3\r\nGET\r\n$1\r\nk\r\n', '$1\r\nv\r\n')

    # conn2 should connect to new instance
    conn2 = get_tcp_conn(nc.host(), nc.port())
    send_cmd(conn2, '*2\r\n$3\r\nGET\r\n$1\r\nk\r\n', '$1\r\nv\r\n')

    # the old connection is still ok in T_RELOAD_DELAY seconds
    send_cmd(conn, '*2\r\n$3\r\nGET\r\n$1\r\nk\r\n', '$1\r\nv\r\n')

    time.sleep(T_RELOAD_DELAY)
    assert(pid != nc.pid())

    # assert the old connection is closed.
    send_cmd(conn, '*2\r\n$3\r\nGET\r\n$1\r\nk\r\n', '')

    # conn2 should survive
    send_cmd(conn2, '*2\r\n$3\r\nGET\r\n$1\r\nk\r\n', '$1\r\nv\r\n')

    r = redis.Redis(nc.host(), nc.port())
    rst = r.set('k', 'v')
    assert(r.get('k') == 'v')

@with_setup(_setup, _teardown)
def test_new_port():
    if nc.version() < '0.4.2':
        print 'Ignore test_reload for version %s' % nc.version()
        return
    r = redis.Redis(nc.host(), nc.port())
    r.set('k', 'v')

    content = '''
reload_test:
  listen: 0.0.0.0:4101
  hash: fnv1a_64
  distribution: modula
  redis: true
  timeout: 400
  servers:
    - 127.0.0.1:2100:1 redis-2100
    - 127.0.0.1:2101:1 redis-2101
'''

    nc.set_config(content)
    time.sleep(T_RELOAD_DELAY)

    r1 = redis.Redis(nc.host(), nc.port())
    r2 = redis.Redis(nc.host(), 4101)

    assert_fail('Connection refused', r1.get, 'k')
    assert(r2.get('k') == 'v')

@with_setup(_setup, _teardown)
def test_pool_add_del():
    if nc.version() < '0.4.2':
        print 'Ignore test_reload for version %s' % nc.version()
        return

    r = redis.Redis(nc.host(), nc.port())

    r.set('k', 'v')
    content = '''
reload_test:
  listen: 0.0.0.0:4100
  hash: fnv1a_64
  distribution: modula
  redis: true
  servers:
    - 127.0.0.1:2100:1 redis-2100
    - 127.0.0.1:2101:1 redis-2101

reload_test2:
  listen: 0.0.0.0:4101
  hash: fnv1a_64
  distribution: modula
  redis: true
  servers:
    - 127.0.0.1:2100:1 redis-2100
    - 127.0.0.1:2101:1 redis-2101
'''

    nc.set_config(content)
    time.sleep(T_RELOAD_DELAY)

    r1 = redis.Redis(nc.host(), nc.port())
    r2 = redis.Redis(nc.host(), 4101)

    assert(r1.get('k') == 'v')
    assert(r2.get('k') == 'v')

    content = '''
reload_test:
  listen: 0.0.0.0:4102
  hash: fnv1a_64
  distribution: modula
  redis: true
  preconnect: true
  servers:
    - 127.0.0.1:2100:1 redis-2100
    - 127.0.0.1:2101:1 redis-2101
'''
    nc.set_config(content)
    time.sleep(T_RELOAD_DELAY)
    pid = nc.pid()
    print system('ls -l /proc/%s/fd/' % pid)

    r3 = redis.Redis(nc.host(), 4102)

    assert_fail('Connection refused', r1.get, 'k')
    assert_fail('Connection refused', r2.get, 'k')
    assert(r3.get('k') == 'v')

    fds = system('ls -l /proc/%s/fd/' % pid)
    sockets = [s for s in fds.split('\n') if strstr(s, 'socket:') ]
    # pool + stat + 2 backend + 1 client
    assert(len(sockets) == 5)

