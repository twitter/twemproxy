#!/bin/bash
#-------------------------------------------------------------------------------
# Create a source RPM package
# Author: Elvin Sindrilaru <esindril@cern.ch> 2015
#-------------------------------------------------------------------------------
RCEXP='^[0-9]+\.[0-9]+\.[0-9]+\-rc.*$'

#-------------------------------------------------------------------------------
# Color console messages
#-------------------------------------------------------------------------------
NORMAL=$(tput sgr0)
GREEN=$(tput setaf 2; tput bold)
RED=$(tput setaf 1)

function green() {
    echo -e "$GREEN$*$NORMAL"
}

function red() {
    echo -e "$RED$*$NORMAL"
}

#-------------------------------------------------------------------------------
# Find a program
#-------------------------------------------------------------------------------
function findProg()
{
  for prog in $@; do
    if test -x "`which $prog 2>/dev/null`"; then
      echo $prog
      break
    fi
  done
}

#-------------------------------------------------------------------------------
# Print help
#-------------------------------------------------------------------------------
function printHelp()
{
  echo "Usage:"                                              1>&2
  echo "${0} [--help] [--source PATH] [--output PATH]"       1>&2
  echo "  --help        prints this message"                 1>&2
  echo "  --source PATH specify the root of the source tree" 1>&2
  echo "                defaults to ../"                     1>&2
  echo "  --output PATH the directory where the source rpm"  1>&2
  echo "                should be stored, defaults to ."     1>&2
}

#-------------------------------------------------------------------------------
# Parse the commandline
#-------------------------------------------------------------------------------
SOURCEPATH="../"
OUTPUTPATH="."
PRINTHELP=0

while test ${#} -ne 0; do
  if test x${1} = x--help; then
    PRINTHELP=1
  elif test x${1} = x--source; then
    if test ${#} -lt 2; then
      echo "--source parameter needs an argument" 1>&2
      exit 1
    fi
    SOURCEPATH=${2}
    shift
  elif test x${1} = x--output; then
    if test ${#} -lt 2; then
      echo "--output parameter needs an argument" 1>&2
      exit 1
    fi
    OUTPUTPATH=${2}
    shift
  else
    PRINTHELP=1
  fi
  shift
done

if test $PRINTHELP -eq 1; then
  printHelp
  exit 0
fi

echo "[i] Working on: $SOURCEPATH"
echo "[i] Storing the output to: $OUTPUTPATH"

#-------------------------------------------------------------------------------
# Check if the source and the output dirs exist
#-------------------------------------------------------------------------------
if test ! -d $SOURCEPATH -o ! -r $SOURCEPATH; then
  red "[!] Source path does not exist or is not readable" 1>&2
  exit 2
fi

if test ! -d $OUTPUTPATH -o ! -w $OUTPUTPATH; then
  red "[!] Output path does not exist or is not writeable" 1>&2
  exit 2
fi

#-------------------------------------------------------------------------------
# Check if we have all the necassary components
#-------------------------------------------------------------------------------
if test x`findProg rpmbuild` = x; then
  red "[!] Unable to find rpmbuild, aborting..." 1>&2
  exit 1
fi

if test x`findProg git` = x; then
  red "[!] Unable to find git, aborting..." 1>&2
  exit 1
fi

if test x`findProg awk` = x; then
  red "[!] Unable to find awk, aborting..." 1>&2
  exit 1
fi

#-------------------------------------------------------------------------------
# Check if the source is a git repository
#-------------------------------------------------------------------------------
if test ! -d $SOURCEPATH/.git; then
  red "[!] I can only work with a git repository" 1>&2
  exit 2
fi

#-------------------------------------------------------------------------------
# Get VERSION from the spec file
#-------------------------------------------------------------------------------
SPEC_FILE="nutcracker.spec"

if test -e "$SPEC_FILE"; then
    VERSION="$(grep "Version:" $SPEC_FILE | awk '{print $2;}')"
else
    red "[!] Unable to get version from spec file." 1>&2
    exit 4
fi

echo "[i] Working with version: $VERSION"

if test x${VERSION:0:1} = x"v"; then
  VERSION=${VERSION:1}
fi

#-------------------------------------------------------------------------------
# Deal with release candidates
#-------------------------------------------------------------------------------
RELEASE=1

if test x`echo $VERSION | egrep $RCEXP` != x; then
  RELEASE=0.`echo $VERSION | sed 's/.*-rc/rc/'`
  VERSION=`echo $VERSION | sed 's/-rc.*//'`
fi

VERSION=`echo $VERSION | sed 's/-/./g'`
echo "[i] RPM compliant version: $VERSION-$RELEASE"

#-------------------------------------------------------------------------------
# Create a tempdir and copy the files there
#-------------------------------------------------------------------------------
# exit on any error
set -e

TEMPDIR=`mktemp -d /tmp/nutcracker.srpm.XXXXXXXXXX`
RPMSOURCES=$TEMPDIR/rpmbuild/SOURCES
mkdir -p $RPMSOURCES
mkdir -p $TEMPDIR/rpmbuild/SRPMS
echo "[i] Working in: $TEMPDIR" 1>&2

#-------------------------------------------------------------------------------
# Make a tarball of the latest commit on the branch
#-------------------------------------------------------------------------------
# no more exiting on error
set +e

CWD=$PWD
cd $SOURCEPATH
COMMIT=`git log --pretty=format:"%H" -1`

if test $? -ne 0; then
  red "[!] Unable to figure out the git commit hash" 1>&2
  exit 5
fi

git archive --prefix=nutcracker-"$VERSION"/ --format=tar $COMMIT | gzip -9fn > \
      $RPMSOURCES/nutcracker-"$VERSION".tar.gz

if test $? -ne 0; then
  red "[!] Unable to create the source tarball" 1>&2
  exit 6
fi

cd $CWD

#-------------------------------------------------------------------------------
# Build the source RPM
#-------------------------------------------------------------------------------
echo "[i] Creating the source RPM..."

# Dirty, dirty hack!
echo "%_sourcedir $RPMSOURCES" >> $TEMPDIR/rpmmacros
rpmbuild --define "_topdir $TEMPDIR/rpmbuild"    \
	 --define "%_sourcedir $RPMSOURCES"      \
	 --define "%_srcrpmdir %{_topdir}/SRPMS" \
	 --define "_source_filedigest_algorithm md5" \
	 --define "_binary_filedigest_algorithm md5" \
  -bs nutcracker.spec > $TEMPDIR/log
if test $? -ne 0; then
  red "[!] RPM creation failed" 1>&2
  exit 8
fi

cp $TEMPDIR/rpmbuild/SRPMS/nutcracker*.src.rpm $OUTPUTPATH
rm -rf $TEMPDIR

green "[i] Done."
