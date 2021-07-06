Summary: Twitter's nutcracker redis and memcached proxy
Name: nutcracker
Version: 0.5.0
Release: 0.1.rc1

URL: https://github.com/twitter/twemproxy/
Source0: %{name}-%{version}.tar.gz
License: Apache License 2.0
Group: System Environment/Libraries
Packager: Tom Parrott <tomp@tomp.co.uk>
BuildRoot: %{_tmppath}/%{name}-root

BuildRequires: autoconf
BuildRequires: automake
BuildRequires: libtool

%description
twemproxy (pronounced "two-em-proxy"), aka nutcracker is a fast and lightweight proxy for memcached and redis protocol.
It was primarily built to reduce the connection count on the backend caching servers. This, together with protocol
pipelining and sharding enables you to horizontally scale your distributed caching architecture.

%prep
%setup -q
%if 0%{?rhel} == 6
sed -i 's/2.64/2.63/g' configure.ac
%endif
autoreconf -fvi

%build

%configure
%__make

%install
[ %{buildroot} != "/" ] && rm -rf %{buildroot}

%makeinstall PREFIX=%{buildroot}

#Install init script
%{__install} -p -D -m 0755 scripts/%{name}.init %{buildroot}%{_initrddir}/%{name}

#Install example config file
%{__install} -p -D -m 0644 conf/%{name}.yml %{buildroot}%{_sysconfdir}/%{name}/%{name}.yml

%post
/sbin/chkconfig --add %{name}

%preun
if [ $1 = 0 ]; then
 /sbin/service %{name} stop > /dev/null 2>&1
 /sbin/chkconfig --del %{name}
fi

%clean
[ %{buildroot} != "/" ] && rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%if 0%{?rhel} >= 6
/usr/sbin/nutcracker
%else
/usr/bin/nutcracker
%endif
%{_initrddir}/%{name}
%{_mandir}/man8/nutcracker.8.gz
%config(noreplace)%{_sysconfdir}/%{name}/%{name}.yml

%changelog
* Tue Jul 06 2021 Tyson Andre  <tysonandre775@hotmail.com>
- twemproxy: version 0.5.0-RC1 release
- Add 'tcpkeepalive' pool boolean config flag setting to enable tcp keepalive (charsyam, manju)
- Support redis bitpos command (clark kang)
- Fix parsing of redis error response for error type with no space, add tests (tyson, tom dalton)
- Update integration tests, add C unit test suite for 'make check' (tyson)
- Increase the maximum host length+port+identifier to 273 in ketama_update (李广博)
- Always initialize file permissions field when listening on a unix domain socket (tyson)
- Use number of servers instead of number of points on the continuum when sharding requests to backend services to improve sharding performance and fix potential invalid memory access when all hosts were ejected from a pool. (tyson)
- Optimize performance of deletion of single redis keys (vincentve)
- Don't fragment memcache/redis get commands when they only have a single key (improves performance and error handling of single key case) (tyson)
- Don't let requests hang when there is a dns error when processing a fragmented request (e.g. multiget with multiple keys) (tyson)
- Allow extra parameters for redis spop (charsyam)
- Update documentation and README (various)
- Fix memory leak bug for redis mset (deep011)
- Support arbitrarily deep nested redis multi-bulk responses (nested arrays) (qingping209, tyson)
- Upgrade from libyaml 0.1.4 to 0.2.5 (tyson)
- Fix compiler warnings about wrong conversion specifiers in format strings for logging (tyson)
- Log the async backend used and any debug options in the '--help'/'--version' output.
- Add support for many more new redis commands and updates to existing redis commands (tyson)
- Optimization: Skip hashing and choosing server index when a pool has exactly one server (tyson)
- Support memcache 'version' requests by proxying the request to a single backend memcache server to fetch the server version. (tyson)
- Make error messages for creating the stats server during startup clearer. (tyson)

* Mon Jun 22 2015  Manju Rajashekhar  <manj@cs.stanford.edu>
- twemproxy: version 0.4.1 release
- backend server hostnames are resolved lazily
- redis_auth is only valid for a redis pool
- getaddrinfo returns non-zero +ve value on error
- fix-hang-when-command-only (charsyam)
- fix bug crash when get command without key and whitespace (charsyam)
- mark server as failed on protocol level transiet failures like -OOM, -LOADING, etc
- implemented support for parsing fine grained redis error response
- remove redundant conditional judgement in rbtree deletion (leo ma)
- fix bug mset has invalid pair (charsyam)
- fix bug mset has invalid pair (charsyam)
- temp fix a core on kqueue (idning)
- support "touch" command for memcached (panmiaocai)
- fix redis parse rsp bug (charsyam)
- SORT command can take multiple arguments. So it should be part of redis_argn() and not redis_arg0()
- remove incorrect assert because client could send data after sending a quit request which must be discarded
- allow file permissions to be set for UNIX domain listening socket (ori liveneh)
- return error if formatted is greater than mbuf size by using nc_vsnprintf() in msg_prepend_format()
- fix req_make_reply on msg_get, mark it as response (idning)
- redis database select upon connect (arne claus)
- redis_auth (charsyam)
- allow null key(empty key) (idning)
- fix core on invalid mset like "mset a a a" (idning)

* Tue Oct 14 2014 idning <idning@gmail.com>
- twemproxy: version 0.4.0 release
- mget improve (idning)
- many new commands supported: LEX, PFADD, PFMERGE, SORT, PING, QUIT, SCAN... (mattrobenolt, areina, idning)
- handle max open file limit(allenlz)
- add notice-log and use ms time in log(idning)
- fix bug in string_compare (andyqzb)
- fix deadlock in sighandler (idning)

* Fri Dec 20 2013  Manju Rajashekhar  <manj@cs.stanford.edu>
- twemproxy: version 0.3.0 release
- SRANDMEMBER support for the optional count argument (mkhq)
- Handle case where server responds while the request is still being sent (jdi-tagged)
- event ports (solaris/smartos) support
- add timestamp when the server was ejected
- support for set ex/px/nx/xx for redis 2.6.12 and up (ypocat)
- kqueue (bsd) support (ferenyx)
- fix parsing redis response to accept integer reply (charsyam)

* Tue Jul 30 2013 Tait Clarridge <tait@clarridge.ca>
- Rebuild SPEC to work with CentOS
- Added buildrequires if building with mock/koji

* Tue Apr 23 2013  Manju Rajashekhar  <manj@cs.stanford.edu>
- twemproxy: version 0.2.4 release
- redis keys must be less than mbuf_data_size() in length (fifsky)
- Adds support for DUMP/RESTORE commands in Redis (remotezygote)
- Use of the weight value in the modula distribution (mezzatto)
- Add support to unix socket connections to servers (mezzatto)
- only check for duplicate server name and not 'host:port:weight' when 'name' is configured
- crc16 hash support added (mezzatto)

* Thu Jan 31 2013  Manju Rajashekhar  <manj@twitter.com>
- twemproxy: version 0.2.3 release
- RPOPLPUSH, SDIFF, SDIFFSTORE, SINTER, SINTERSTORE, SMOVE, SUNION, SUNIONSTORE, ZINTERSTORE, and ZUNIONSTORE support (dcartoon)
- EVAL and EVALSHA support (ferenyx)
- exit 1 if configuration file is invalid (cofyc)
- return non-zero exit status when nutcracker cannot start for some reason
- use server names in stats (charsyam)
- Fix failure to resolve long FQDN name resolve (conmame)
- add support for hash tags

* Thu Oct 18 2012  Manju Rajashekhar  <manj@twitter.com>
- twemproxy: version 0.2.2 release
- fix the off-by-one error when calculating redis key length

* Fri Oct 12 2012  Manju Rajashekhar  <manj@twitter.com>
- twemproxy: version 0.2.1 release
- don't use buf in conf_add_server
- allow an optional instance name for consistent hashing (charsyam)
- add --stats-addr=S option
- add stats-bind-any -a option (charsyam)
