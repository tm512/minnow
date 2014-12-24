#!/bin/sh

[ ! -d .git ] && echo "nongit-$(date +"%Y%m%d")" && exit

git log -n1 HEAD --date=short --pretty=%h-%ad | sed 's/\(....\)-\(..\)-\(..\)$/\1\2\3/'
