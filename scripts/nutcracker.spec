Summary: Twitter's nutcracker redis and memcached proxy
Name: nutcracker
Version: 0.2.4
Release: 1

URL: http://code.google.com/p/twemproxy/
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
It was primarily built to reduce the connection count on the backend caching servers.

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

#Install example confog file
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
/usr/sbin/nutcracker
%{_initrddir}/%{name}
%{_mandir}/man8/nutcracker.8.gz
%config(noreplace)%{_sysconfdir}/%{name}/%{name}.yml

%changelog
* Tue July 30 2013 Tait Clarridge <tait@clarridge.ca>
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
