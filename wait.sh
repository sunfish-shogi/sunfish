#!/bin/bash

echo waiting for $1

while :
do
	pnum=`ps -A | grep $1 | wc -l`

	if [ $pnum -eq 0 ]
	then
		break
	fi

	sleep 1
done
