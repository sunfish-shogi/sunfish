#!/bin/bash

while :
do
	echo "copy $1 => $2"

	expect -c "
	spawn scp -o ConnectTimeout=60 ${1} ${2}
	expect {
		\"Are you sure you want to continue connecting (yes/no)?\" {
			send \"yes\n\"
			expect \"password:\"
			send \"${3}\n\"
		} \"password:\" {
			send \"${3}\n\"
		}
	}
	interact
	" > scp.log

	scp=`grep '%' scp.log | wc -l`
	scpok=`grep '100%' scp.log | wc -l`

	if [ $scpok -eq $scp ]
	then
		echo ok!
		break
	else
		echo error!
	fi

	sleep 5
	echo retry!
done
