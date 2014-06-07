#!/bin/sh

if [ -d .git ] ; then
	out="$(git rev-parse --short HEAD)"

	if [ $(uname | grep -i bsd) ] ; then
		echo "bsd"
		out="${out}-$(date -j -f "%+" "$(git log -n1 $out | grep "^Date" | sed 's/Date:   //;s/-[0-9]*//')" +"%Y%m%d")"
	else
		echo "linux"
		out="${out}-$(date -d"$(git log -n1 $out | grep "^Date" | sed 's/Date:   //;s/-[0-9]*//')" +"%Y%m%d")"
	fi

	echo "$out"
else
	echo "nongit-$(date +"%Y%m%d")"
fi
