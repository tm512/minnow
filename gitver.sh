#!/bin/sh

if [ -d .git ] ; then
	out="$(git rev-parse --short HEAD)"

	if [ $(uname | grep -iE (bsd|dragonfly)) ] ; then
		out="${out}-$(date -j -f "%+" "$(git log -n1 $out | grep "^Date" | sed 's/Date:   //;s/-[0-9]*//')" +"%Y%m%d")"
	else
		out="${out}-$(date -d"$(git log -n1 $out | grep "^Date" | sed 's/Date:   //;s/-[0-9]*//')" +"%Y%m%d")"
	fi

	echo "$out"
else
	echo "nongit-$(date +"%Y%m%d")"
fi
