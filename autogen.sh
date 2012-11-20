#!/bin/sh

function assert_command() {
  TEST=$(which $1 2>/dev/null)
  if [[ $? -eq 0 ]]; then
    return;
  else
    echo "Command '$1' missing"
    exit 1
  fi
}

function cleanup() {
  if [ -f Makefile ]; then
    make distclean
  fi

  rm -f Makefile.in \
	  aclocal.m4 \
	  compile \
	  config.h.in \
	  configure \
	  depcomp \
	  install-sh \
	  missing \
	  src/Makefile.in \
    *~ \
    m4/*

  rm -rf autom4te.cache
}

function autogen() {
  assert_command 'autoreconf'
  exec autoreconf -ivf

  LIBTOOLIZE=libtoolize
  SYSNAME=$(uname)
  if [ "x$SYSNAME" = "xDarwin" ] ; then
    LIBTOOLIZE=glibtoolize
  fi

  assert_command 'automake'
  assert_command $LIBTOOLIZE
  assert_command 'aclocal'
  assert_command 'autoheader'
  assert_command 'autoconf'

  aclocal -I m4 && \
	  autoheader && \
	  $LIBTOOLIZE && \
	  autoconf && \
	  automake --add-missing --force-missing --copy
}

function usage() {
cat << EOF
usage: $0 [options]

Create configure script using autotools, or cleanup

OPTIONS:
  -h Show this message
  -c Cleanup
EOF
}

while getopts "hc" OPTION
do
  case $OPTION in
    c)
      cleanup
      exit
      ;;
    ?)
      usage
      exit
      ;;
  esac
done

autogen
