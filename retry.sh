#!/bin/bash

while :
do
	echo $1
	$1

	if [ $? -eq 0 ]
	then
		break
	fi

	echo "retry!!"

	echo $1 was down                      >> retry.log
	date                                  >> retry.log
	echo '------------------------------' >> retry.log

	sleep 1
done
