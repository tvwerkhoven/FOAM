#!/bin/bash

if [ "`svn info | grep ^Revision`" ] ; then
	echo "hi"
fi
#echo "r"`svn info | grep ^Revision | cut -f 2 -d' '`
