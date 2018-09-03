#!/bin/bash
HERE="$(dirname "$(readlink -f "$0")")"
cd "$HERE"

FVP_CMD=Foundation_Platform

[ -e env.sh ] && . env.sh

if [ -z `which $FVP_CMD` ] ; then
	echo "Error: command $FVP_CMD not found" >&2
	exit 1
fi

$FVP_CMD --arm-v8.0 \
	 --cores=4 \
	 --secure-memory \
	 --visualization \
	 --data="bl1.bin"@0x0 \
	 --data="fip.bin"@0x8000000

