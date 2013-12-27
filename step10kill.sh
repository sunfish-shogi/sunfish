#!/bin/bash

while :
do
	step10=`grep '^Step .* : 10$' evinfo | wc -l`
	if [ $step10 -gt 0 ]
	then
		break
	fi
	echo unreached step 10
	sleep 1
done
sleep 10
