#!/bin/bash
# Automatic git revision inclusion, base on
# <http://groups.google.com/group/memcached/browse_thread/thread/b02e992ede0f0e89>
VER=`git describe HEAD 2>/dev/null`
if [ "$VER" ] ; then
	echo "m4_define([GIT_REVISION], [`echo $VER | tr -d '\n'`])" > version.m4
fi
