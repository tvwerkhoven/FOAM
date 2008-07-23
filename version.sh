#!/bin/bash

if [ "`svn info | grep ^Revision`" ] ; then
## If SVN gives a decent value back (i.e. non-null), use that
	echo "r"`svn info | grep ^Revision | cut -f 2 -d' '`
else
## Otherwise use a manual version number
	echo "r550"
fi
