#!/bin/bash
VER=`git describe HEAD 2>/dev/null`
if [ "$VER" ] ; then
	echo "m4_define([GIT_REVISION], [`echo $VER | tr -d '\n'`])" > version.m4
fi
