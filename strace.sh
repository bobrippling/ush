#!/bin/sh

while :
do
	pid=`pgrep '\<ush\>'`
	if [ -n "$pid" ] && ! strace -p $pid
	then exit
	fi
	sleep 1
done
