#!/usr/bin/env python3
from .common import *
from pprint import pprint

def get_conn():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((nc.host(), nc.port()))
    s.settimeout(.3)
    return s

def _test(req, resp, sleep=0):
    s = get_conn()

    # Send a single byte at a time
    for i in req:
        s.sendall(bytes([i]))
        time.sleep(sleep)

    s.settimeout(.3)

    data = s.recv(10000)
    assert_equal(resp, data)

def test_slow():
    req = b'*1\r\n$4\r\nPING\r\n'
    resp = b'+PONG\r\n'

    if large > 1000:
        sleep = 1
    else:
        sleep = .1

    _test(req, resp, sleep)

def test_pingpong():
    req = b'*1\r\n$4\r\nPING\r\n'
    resp = b'+PONG\r\n'
    _test(req, resp)
    # Sanity check there's no error
    info = nc._info_dict()
    assert_equal(0, info['ntest']['client_err'])

# twemproxy for redis doesn't appear to have any code to send +OK\r\n, it just disconnects.
def test_quit():
    req = b'*1\r\n$4\r\nQUIT\r\n'
    # NOTE: Nutcracker doesn't appear to have any code to send +OK\r\n, it just disconnects.
    # +OK\r\n would also be valid.
    resp = b''
    _test(req, resp)

# twemproxy for redis doesn't appear to have any code to send +OK\r\n, it just disconnects.
# If it doesn't try to send anything, there's no client_err.
def test_quit_without_recv():
    req = b'*1\r\n$4\r\nQUIT\r\n'
    resp = b'+OK\r\n'
    s = get_conn()

    s.sendall(req)
    s.close()
    time.sleep(0.1)
    info = nc._info_dict()
    assert_equal(0, info['ntest']['client_err'])

def _test_bad(req):
    s = get_conn()

    s.sendall(req)
    data = s.recv(10000)

    assert_equal(b'', s.recv(1000))  # peer is closed

def test_badreq():
    reqs = [
        # '*1\r\n$3\r\nPING\r\n',
        b'\r\n',
        # '*3abcdefg\r\n',
        b'*3\r\n*abcde\r\n',

        b'*4\r\n$4\r\nMSET\r\n$1\r\nA\r\n$1\r\nA\r\n$1\r\nA\r\n',
        b'*2\r\n$4\r\nMSET\r\n$1\r\nA\r\n',
        # '*3\r\n$abcde\r\n',
        # '*3\r\n$3abcde\r\n',
        # '*3\r\n$3\r\nabcde\r\n',
    ]

    for req in reqs:
        _test_bad(req)


def test_wrong_argc():
    s = get_conn()

    s.sendall(b'*1\r\n$3\r\nGET\r\n')
    assert_equal(b'', s.recv(1000))  # peer is closed
