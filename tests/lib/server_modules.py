#!/usr/bin/env python
#coding: utf-8
#file   : server_modules.py
#author : ning
#date   : 2014-02-24 13:00:28

import os
import sys

from utils import *
import conf

if sys.version_info[0] < 3:
    # Give a clear error message instead of a confusing one.
    sys.stderr.write("Error: must use python 3 to run these nosetests, e.g. python3 -m nose [options] test_modules\n")
    sys.exit(2)

class Base:
    '''
    Sub class should implement:
    _alive, _pre_deploy, status, and init self.args
    '''
    def __init__(self, name, host, port, path):
        self.args = {
            'name'      : name,
            'host'      : host,
            'port'      : port,
            'path'      : path,

            #startcmd and runcmd will used to generate the control script
            #used for the start cmd
            'startcmd'  : '',
            #process name you see in `ps -aux`, used this to generate stop cmd
            'runcmd'    : '',
            'logfile'   : '',
        }

    def __str__(self):
        return TT('[$name:$host:$port]', self.args)

    def deploy(self):
        logging.info('deploy %s' % self)
        self._run(TTCMD('mkdir -p $path/bin &&  \
                      mkdir -p $path/conf && \
                      mkdir -p $path/log &&  \
                      mkdir -p $path/data',
                     self.args))

        self._pre_deploy()
        self._gen_control_script()

    def _gen_control_script(self):
        content = open(os.path.join(WORKDIR, 'conf/control.sh'), 'r').read()
        content = TT(content, self.args)

        control_filename = TT('${path}/${name}_control', self.args)

        fout = open(control_filename, 'w+')
        fout.write(content)
        fout.close()
        os.chmod(control_filename, 0o755)

    def start(self):
        if self._alive():
            logging.warning('%s already running' % (self))
            return

        logging.debug('starting %s' % self)
        t1 = time.time()
        sleeptime = .1

        cmd = TT("cd $path && ./${name}_control start", self.args)
        self._run(cmd)

        while not self._alive():
            lets_sleep(sleeptime)
            if sleeptime < 5:
                sleeptime *= 2
            else:
                sleeptime = 5.0
                logging.warning('%s still not alive' % self)

        t2 = time.time()
        logging.info('%s start ok in %.2f seconds' %(self, t2-t1))

    def stop(self):
        if not self._alive():
            logging.warning('%s already stop' %(self))
            return

        cmd = TT("cd $path && ./${name}_control stop", self.args)
        self._run(cmd)

        t1 = time.time()
        while self._alive():
            lets_sleep()
        t2 = time.time()
        logging.info('%s stop ok in %.2f seconds' %(self, t2-t1))

    def pid(self):
        cmd = TT("pgrep -f '^$runcmd'", self.args)
        return self._run(cmd)

    def status(self):
        logging.warning("status: not implement")

    def _alive(self):
        logging.warning("_alive: not implement")

    def _run(self, raw_cmd):
        ret = system(raw_cmd, logging.debug)
        logging.debug('return : [%d] [%s] ' % (len(ret), shorten(ret)))
        return ret

    def clean(self):
        cmd = TT("rm -rf $path", self.args)
        self._run(cmd)

    def host(self):
        return self.args['host']

    def port(self):
        return self.args['port']

class RedisServer(Base):
    def __init__(self, host, port, path, cluster_name, server_name, auth = None):
        Base.__init__(self, 'redis', host, port, path)

        self.args['startcmd']     = TT('bin/redis-server conf/redis.conf', self.args)
        self.args['runcmd']       = TT('redis-server \\*:$port', self.args)
        self.args['conf']         = TT('$path/conf/redis.conf', self.args)
        self.args['pidfile']      = TT('$path/log/redis.pid', self.args)
        self.args['logfile']      = TT('$path/log/redis.log', self.args)
        self.args['dir']          = TT('$path/data', self.args)
        self.args['REDIS_CLI']    = conf.BINARYS['REDIS_CLI']

        self.args['cluster_name'] = cluster_name
        self.args['server_name']  = server_name
        self.args['auth']         = auth

    def _info_dict(self):
        cmd = TT('$REDIS_CLI -h $host -p $port INFO', self.args)
        if self.args['auth']:
            cmd = TT('$REDIS_CLI -h $host -p $port -a $auth INFO', self.args)
        info = self._run(cmd)

        info = [line.split(':', 1) for line in info.split('\r\n')
                if not line.startswith('#')]
        info = [i for i in info if len(i) > 1]
        return defaultdict(str, info) #this is a defaultdict, be Notice

    def _ping(self):
        cmd = TT('$REDIS_CLI -h $host -p $port PING', self.args)
        if self.args['auth']:
            cmd = TT('$REDIS_CLI -h $host -p $port -a $auth PING', self.args)
        return self._run(cmd)

    def _alive(self):
        return strstr(self._ping(), 'PONG')

    def _gen_conf(self):
        content = open(os.path.join(WORKDIR, 'conf/redis.conf'), 'r').read()
        content = TT(content, self.args)
        if self.args['auth']:
            content += '\r\nrequirepass %s' % self.args['auth']
        return content

    def _pre_deploy(self):
        self.args['BINS'] = conf.BINARYS['REDIS_SERVER_BINS']
        self._run(TT('cp $BINS $path/bin/', self.args))

        fout = open(TT('$path/conf/redis.conf', self.args), 'w+')
        fout.write(self._gen_conf())
        fout.close()

    def status(self):
        uptime = self._info_dict()['uptime_in_seconds']
        if uptime:
            logging.info('%s uptime %s seconds' % (self, uptime))
        else:
            logging.error('%s is down' % self)

    def isslaveof(self, master_host, master_port):
        info = self._info_dict()
        if info['master_host'] == master_host and \
           int(info['master_port']) == master_port:
            logging.debug('already slave of %s:%s' % (master_host, master_port))
            return True

    def slaveof(self, master_host, master_port):
        cmd = 'SLAVEOF %s %s' % (master_host, master_port)
        return self.rediscmd(cmd)

    def rediscmd(self, cmd):
        args = copy.deepcopy(self.args)
        args['cmd'] = cmd
        cmd = TT('$REDIS_CLI -h $host -p $port $cmd', args)
        logging.info('%s %s' % (self, cmd))
        return self._run(cmd)

class RedisSentinel(RedisServer):
    def __init__(self, host, port, path, cluster_name, server_name, masters, quorum, down_time, auth = None):
        RedisServer.__init__(self, host, port, path, cluster_name, server_name, auth)

        self.masters = masters
        self.quorum = quorum
        self.down_time = down_time

        self.args['startcmd']     = TT('bin/redis-sentinel conf/sentinel.conf', self.args)
        self.args['runcmd']       = TT('redis-sentinel \\*:$port', self.args)
        self.args['conf']         = TT('$path/conf/sentinel.conf', self.args)
        self.args['pidfile']      = TT('$path/log/sentinel.pid', self.args)
        self.args['logfile']      = TT('$path/log/sentinel.log', self.args)

    def _gen_conf_section(self):
        template = '''
sentinel monitor $server_name $host $port %d
sentinel down-after-milliseconds $server_name %d
sentinel parallel-syncs $server_name 1
sentinel failover-timeout $server_name 180000
''' % (self.quorum, self.down_time)
        cfg = '\n'.join([TT(template, master.args) for master in self.masters])
        return cfg

    def _gen_conf(self):
        content = open(os.path.join(WORKDIR, 'conf/sentinel.conf'), 'r').read()
        content = TT(content, self.args)
        if self.args['auth']:
            content += '\r\nrequirepass %s' % self.args['auth']
        return content + self._gen_conf_section()

    def _pre_deploy(self):
        self.args['BINS'] = conf.BINARYS['REDIS_SERVER_BINS']
        self._run(TT('cp $BINS $path/bin/', self.args))

        fout = open(TT('$path/conf/sentinel.conf', self.args), 'w+')
        fout.write(self._gen_conf())
        fout.close()

    def failover(self, server_name):
        cmd = 'SENTINEL FAILOVER %s' % server_name
        return self.rediscmd(cmd)

class Memcached(Base):
    def __init__(self, host, port, path, cluster_name, server_name):
        Base.__init__(self, 'memcached', host, port, path)

        self.args['startcmd']     = TT('bin/memcached -d -p $port', self.args)
        self.args['runcmd']       = self.args['startcmd']

        self.args['cluster_name'] = cluster_name
        self.args['server_name']  = server_name

    def _alive(self):
        cmd = TT('echo "stats" | socat - TCP:$host:$port', self.args)
        ret = self._run(cmd)
        return strstr(ret, 'END')

    def _pre_deploy(self):
        self.args['BINS'] = conf.BINARYS['MEMCACHED_BINS']
        self._run(TT('cp $BINS $path/bin/', self.args))

class NutCracker(Base):
    def __init__(self, host, port, path, cluster_name, masters, mbuf=512,
            verbose=5, is_redis=True, redis_auth=None, sentinels=None):
        Base.__init__(self, 'nutcracker', host, port, path)

        self.masters = masters
        self.sentinels = sentinels

        self.args['mbuf']        = mbuf
        self.args['verbose']     = verbose
        self.args['redis_auth']  = redis_auth
        self.args['conf']        = TT('$path/conf/nutcracker.conf', self.args)
        self.args['pidfile']     = TT('$path/log/nutcracker.pid', self.args)
        self.args['logfile']     = TT('$path/log/nutcracker.log', self.args)
        self.args['status_port'] = self.args['port'] + 1000

        self.args['startcmd'] = TTCMD('bin/nutcracker -d -c $conf -o $logfile \
                                       -p $pidfile -s $status_port            \
                                       -v $verbose -m $mbuf -i 1', self.args)
        self.args['runcmd']   = TTCMD('bin/nutcracker -d -c $conf -o $logfile \
                                       -p $pidfile -s $status_port', self.args)

        self.args['cluster_name']= cluster_name
        self.args['is_redis']= str(is_redis).lower()

    def _alive(self):
        return self._info_dict()

    def _gen_conf_section(self, servers):
        template = '    - $host:$port:1 $server_name'
        cfg = '\n'.join([TT(template, server.args) for server in servers])
        return cfg

    def _gen_conf(self):
        content = '''
$cluster_name:
  listen: 0.0.0.0:$port
  hash: fnv1a_64
  distribution: modula
  preconnect: true
  auto_eject_hosts: false
  redis: $is_redis
  backlog: 512
  timeout: 400
  client_connections: 0
  server_connections: 1
  server_retry_timeout: 2000
  server_failure_limit: 2
  servers:
'''
        if self.args['redis_auth']:
            content = content.replace('redis: $is_redis',
                    'redis: $is_redis\r\n  redis_auth: $redis_auth')
        content = TT(content, self.args)
        content += self._gen_conf_section(self.masters)
        if self.args['is_redis'] and self.sentinels:
            content += '''
  sentinels:
'''
            content += self._gen_conf_section(self.sentinels)
        return content

    def _pre_deploy(self):
        self.args['BINS'] = conf.BINARYS['NUTCRACKER_BINS']
        self._run(TT('cp $BINS $path/bin/', self.args))

        fout = open(TT('$path/conf/nutcracker.conf', self.args), 'w+')
        fout.write(self._gen_conf())
        fout.close()

    def version(self):
        #This is nutcracker-0.4.0
        s = self._run(TT('$BINS --version', self.args))
        return s.strip().replace('This is nutcracker-', '')

    def _info_dict(self):
        try:
            c = telnetlib.Telnet(self.args['host'], self.args['status_port'])
            ret = c.read_all()
            return json_decode(ret)
        except Exception as e:
            logging.debug('can not get _info_dict of nutcracker, \
                          [Exception: %s]' % (e, ))
            return None

    def reconfig(self, masters, sentinels=None):
        self.masters = masters
        if sentinels:
            self.sentinels = sentinels
        self.stop()
        self.deploy()
        self.start()
        logging.info('proxy %s:%s is updated' % (self.args['host'], self.args['port']))

    def logfile(self):
        return self.args['logfile']

    def cleanlog(self):
        cmd = TT("rm '$logfile'", self.args)
        self._run(cmd)

    def signal(self, signo):
        self.args['signo'] = signo
        cmd = TT("pkill -$signo -f '^$runcmd'", self.args)
        self._run(cmd)

    def reload(self):
        self.signal('USR1')

    def set_config(self, content):
        fout = open(TT('$path/conf/nutcracker.conf', self.args), 'w+')
        fout.write(content)
        fout.close()

        self.reload()

