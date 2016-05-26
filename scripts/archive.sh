#!/bin/sh
#files:=$(find . -regex ".*\.\(dll\|so\|dylib\|exe\)")
files=$(find obj/ -executable -type f)
echo "$files" | xargs tar -czvf $1
