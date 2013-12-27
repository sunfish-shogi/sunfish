#!/bin/bash

while :
do
	line=`ps -A -o pid,etime,command | grep " scp " | grep -v grep`
	if [ "x${line}" != "x" ]
	then
		echo ${line}

		pid=`echo ${line} | sed 's/^\([0-9]*\) .*$/\1/'`
		sec=`echo ${line} | sed 's/^[0-9]* [-:0-9]\+:\([0-9]\+\) .*$/\1/'`
		min=`echo ${line} | sed 's/^[0-9]* \([0-9]\+\):[0-9]\+ .*$/\1/'`
		cmd=`echo ${line} | sed 's/^[^ ]* [^ ]* \([^ ]* [^ ]* [^ ]*\) .*$/\1/'`
		echo pid=${pid} min=${min} sec=${sec}
		echo cmd=${cmd}
		if [ ${min} -ge 1 -o ${sec} -ge 20 ]
		then
			kill ${pid}
			${cmd} &
		fi
	fi
	sleep 1
done
