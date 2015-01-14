#!/bin/bash
HERE="$(dirname "$(readlink -f "$0")")"
cd "$HERE"

FVP_CMD=FVP_Base_AEMv8A-AEMv8A

[ -e env.sh ] && . env.sh

if [ -z `which $FVP_CMD` ] ; then
	echo "Error: command $FVP_CMD not found" >&2
	exit 1
fi

$FVP_CMD -C pctl.startup=0.0.0.0 \
	-C bp.secure_memory=1 \
	-C bp.tzc_400.diagnostics=1 \
	-C cluster0.NUM_CORES=2 \
	-C cluster1.NUM_CORES=2 \
	-C cache_state_modelled=0 \
 	-C bp.pl011_uart0.untimed_fifos=1 \
	-C bp.secureflashloader.fname=bl1.bin \
	-C bp.flashloader0.fname=fip.bin

