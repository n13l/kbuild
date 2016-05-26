#!/bin/sh
#files:=$(find . -regex ".*\.\(dll\|so\|dylib\|exe\)")
files=$(find obj/ -executable -type f)
arch=$(file obj/sysconfig | cut -f2 -d",")
echo "$files" | xargs tar -czvf $1
