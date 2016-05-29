#!/bin/sh
testlib=$(find /usr/arm-linux-gnueabi/ -type f -exec test -x {} \; -print | grep ld)
armlib=$(find /usr/arm-linux-gnueabi/ -type f -exec test -x {} \; -print | grep ld)
armdir=$(dirname "$armlib")
printf "qemu-arm-static $armlib --library-path $armdir"
