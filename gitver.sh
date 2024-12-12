#!/bin/sh

[ ! -d .git ] && echo "nongit-$(date +"%Y%m%d")" && exit

git log -n1 HEAD --date=format:%Y.%m.%d --pretty=%ad-%h
