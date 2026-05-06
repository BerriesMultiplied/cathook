#!/bin/bash

LOG=logs/main.log

if [ $EUID != 0 ]; then
	echo "0"
	exit
fi

mkdir -p logs
$(which node || which nodejs) app.js >$LOG
