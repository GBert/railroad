#!/bin/sh

ID=`id -u`
if test $ID -ne "0"; then
        echo "Need to be root"
        exit 1
fi

if test $# != 3; then
        echo "try: at24.sh BUS PORT DEVICE"
	echo "eg. at24.sh 1 0x50 24c02"
        exit 1
fi

modprobe at24 2>/dev/null

BUS=$1
PORT=$2
CHIP=$3

echo "$CHIP $PORT" > /sys/class/i2c-adapter/i2c-${BUS}/new_device
