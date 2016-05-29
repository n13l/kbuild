#!/bin/sh
armlib=$(find /usr/arm-linux-gnueabi/libhf/ -type f -exec test -x {} \; -print | grep ld)
printf "qemu-arm-static $armlib --library-path /usr/arm-linux-gnueabi/libhf"
