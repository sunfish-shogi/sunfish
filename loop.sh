#!/bin/bash

while :
do
	echo $1
	$1

	echo ----- $1 was down -----

	echo $1 was down                      >> loop.log
	date                                  >> loop.log
	echo '------------------------------' >> loop.log

	sleep 1
done
