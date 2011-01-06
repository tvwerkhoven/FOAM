#!/bin/bash
# Automatic git revision inclusion, base on
# <http://groups.google.com/group/memcached/browse_thread/thread/b02e992ede0f0e89>
VER=`git describe HEAD 2>/dev/null`
if [ "$VER" ] ; then
	echo "m4_define([GIT_REVISION], [`echo $VER | tr -d '\n'`])" > version.m4
	test -f docs/doxyfile &&  sed -s --in-place=.old "s/^PROJECT_NUMBER.*/PROJECT_NUMBER = $VER/" docs/doxyfile
	test -f docs/doxyfile &&  sed -s --in-place=.old "s/^PROJECT_NUMBER.*/PROJECT_NUMBER = $VER/" docs/doxyfile-debug
fi
