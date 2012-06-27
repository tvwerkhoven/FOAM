#!/bin/bash
# Automatic git revision inclusion, base on
# <http://groups.google.com/group/memcached/browse_thread/thread/b02e992ede0f0e89>
VER=`git describe HEAD 2>/dev/null`
BRNCH=`git branch | grep ^\* | awk '{print $2}'`
LASTLOG=`git log --pretty=format:'%h -%d %s %cr' --abbrev-commit --date=relative --max-count=1`
if [ "$VER" ] ; then
	echo "m4_define([GIT_REVISION], [`echo $VER | tr -d '\n'`])" > version.m4
	echo "FOAM_VERSION=\"`echo $VER | tr -d '\n'`\"" > version.mk
	echo "FOAM_BRANCH=\"`echo $BRNCH`\"" >> version.mk
	echo "FOAM_LASTLOG=\"`echo $LASTLOG`\"" >> version.mk
	#test -f docs/doxyfile-debug.base &&  sed -e "s/^PROJECT_NUMBER.*/PROJECT_NUMBER = $VER/" docs/doxyfile-debug.base > docs/doxyfile-debug
fi
