#! /bin/sh

### BEGIN INIT INFO
# Provides:		nutcracker
# Required-Start:	$remote_fs $network
# Required-Stop:	$remote_fs
# Should-Start:		$time $named
# Should-Stop:		
# Default-Start:	2 3 4 5
# Default-Stop:		0 1 6
# Short-Description:	nutcracker
# Description:		Fast, light-weight proxy for memcached and Redis
### END INIT INFO

PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
DAEMON=/usr/sbin/nutcracker
NAME="nutcracker"
DESC="memcached and redis proxy"
PIDFILE=/run/nutcracker/nutcracker.pid
CONFFILE=/etc/nutcracker/nutcracker.yml

. /lib/lsb/init-functions

test -x $DAEMON || exit 0

[ -f /etc/default/$NAME ] && . /etc/default/$NAME

DAEMON_OPTS="$DAEMON_OPTS -d"

case "$1" in
  start)
	if pidofproc -p $PIDFILE $DAEMON > /dev/null; then
		log_failure_msg "Starting $NAME (already started)"
		exit 0
	fi
	if [ ! -e $CONFFILE ]; then
		log_daemon_msg "Not starting $NAME" "$DESC"
		log_progress_msg "(unconfigured)"
		log_end_msg 0
		exit 0
	fi
	if ! $DAEMON -t $DAEMON_OPTS 2> /dev/null; then
		log_failure_msg "Checking $NAME config syntax"
		exit 1
	fi
	log_daemon_msg "Starting $NAME" "$DESC"
	mkdir -p /run/nutcracker
	chown nutcracker:nutcracker /run/nutcracker
	chmod 0755 /run/nutcracker
	start-stop-daemon --quiet --start --chuid nutcracker \
		--pidfile $PIDFILE --exec $DAEMON \
		-- $DAEMON_OPTS
	log_end_msg $?
	;;
  stop)
	log_daemon_msg "Stopping $NAME" "$DESC"
	start-stop-daemon --quiet --stop --retry 5 \
		--pidfile $PIDFILE --exec $DAEMON
	case "$?" in
		0) rm -f $PIDFILE; log_end_msg 0 ;;
		1) log_progress_msg "(already stopped)"
		   log_end_msg 0 ;;
		*) log_end_msg 1 ;;
	esac
	;;
  reload)
	log_daemon_msg "Reloading $NAME" "$DESC"
	start-stop-daemon --quiet --stop --signal HUP --oknodo \
		--pidfile $PIDFILE --exec $DAEMON
	case "$?" in
		0) log_end_msg 0 ;;
		1) log_progress_msg "(not running)"
		   log_end_msg 0 ;;
		*) log_end_msg 1 ;;
	esac
	;;
  force-reload|restart)
	if ! $DAEMON -t $DAEMON_OPTS 2> /dev/null; then
		log_failure_msg "Checking $NAME config syntax"
		exit 1
	fi
	$0 stop
	$0 start
	;;
  status)
	status_of_proc -p $PIDFILE $DAEMON $NAME && exit 0 || exit $?
	;;
  *)
	echo "Usage: ${0} {start|stop|reload|force-reload|restart|status}" >&2
	exit 1
	;;
esac
