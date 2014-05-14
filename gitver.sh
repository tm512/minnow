#!/bin/sh

if [ -d .git ] ; then
	out="$(git rev-parse --short HEAD)"
	out="${out}-$(date -d"$(git log -n1 $out | grep "^Date" | sed 's/Date:   //;s/-[0-9]*//')" +"%Y%m%d")"
	echo "$out"
else
	echo "nongit-$(date +"%Y%m%d")"
fi
