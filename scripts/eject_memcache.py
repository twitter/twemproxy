#!/usr/bin/python env

from optparse import OptionParser
import os, sys, socket, re
import json, memcache
import logging as log
from contextlib import closing

'''
 desc: flushing memcache instance when it ejected in twemproxy cache pool
 author: ugur engin

 usage: tw_eject_memcache [options]

 Options:
   -h, --help  show this help message and exit
   -H HOST     Twemproxy host address
   -P PORT     Twemproxy port

 Also you should define the "cache pool name" in this script where is set in twemproxy service.
'''

def opt():
    parser = OptionParser()
    parser.add_option("-H", dest="host", help="Twemproxy host address")
    parser.add_option("-P", dest="port", type='int', help="Twemproxy port")
    (options, args) = parser.parse_args()
    return options, args

# logging function
def log_script():
    log.basicConfig(
        format="%(asctime)s %(levelname)s %(filename)s %(message)s",
        filename = "/var/log/messages",
        filemode = 'a',
        level = log.DEBUG)

if __name__ == "__main__":
     log_script()
     log_script()

# checking first requirements to connect twemproxy.
def check_opt(host, port):
    if not host:
        print "Unknown: Twemproxy host address is required."
        raise SystemExit, 3
    elif port is None:
        print "Unknown: Twemproxy port number is required."
        raise SystemExit, 3

# checking twemproxy statistics socket port.
def twemproxy_socket(host, port, timeout=3):
    s = socket.socket()
    s.settimeout(timeout)
    try:
        s.connect((host, port))
        return True
    except socket.error, e:
        msg = "Connection to %s on port %s failed: %s" % (host, port, e)
        print msg
        log.debug(msg)
        return False

# get hostname of local memcache
def memcache_host():
    h = socket.gethostname()
    try:
        name = '.'.join(h.split('.')[0:1])
        return name
    except ImportError, e:
        msg= "cannot to take memcache hostname!"
        print msg
        log.debug(msg)
        raise SystemExit, 1

# flushing local memcache
def mc_clean():
    try:
        mc = memcache.Client(["localhost:11211"], debug=1)
        return mc.flush_all()
    except Exception as m:
        log.Exception(m)
        raise SystemExit, 1

# validating json data which returned twemproxy statistic socket.
def validate_ejected_ts(host, port, name):
    s = socket.socket()
    char=[];
    ts_file = "/var/log/tw_ejected_timestamp"
    try:
         s.connect((host, port))
         jrecv = s.recv(10240) # data lenght
         char.append(jrecv)
         jobjc = json.loads(jrecv)
         ts_key = jobjc['<pool-name>'][name]['server_ejected_at']
         if ts_key is not None:
            if (not os.path.exists(ts_file) or os.stat(ts_file).st_size == 0) and ts_key != 0:
                mc_clean()
                write_ts_a = open(ts_file,"a")
                write_ts_a.write(str(ts_key) + "\n")
                write_ts_a.close()
                msg = "mc_flush_1: memcache is flushed due to ejected in twemproxy!"
                print msg
                log.debug(msg)
            elif os.stat(ts_file).st_size != 0 and ts_key != 0:
                with open(ts_file) as f_ts:
                    ts_r = int((list(f_ts)[-1]).strip())
                    if ts_r != ts_key:
                       mc_clean()
                       write_ts_b = open(ts_file,"a")
                       write_ts_b.write(str(ts_key) + "\n")
                       write_ts_b.close()
                       msg = "mc_flush_2: memcache is flushed due to ejected in twemproxy!"
                       print msg
                       log.debug(msg)
                    else:
                     print "OK2: Memcache server is not ejected from twemproxy."
            else:
             print "OK1: Memcache server is not ejected from twemproxy."
    except socket.error, e:
        print "Failed to load json data" % (host, port, e)
        log.debug(e)
        return False

def check_main():
    options, args = opt()
    host = memcache_host()
    check_opt(options.host, options.port)
    twemproxy_socket(options.host, options.port)
    validate_ejected_ts(options.host,options.port,host)

if __name__ == "__main__":
     check_main()
