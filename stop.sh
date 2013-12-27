#/bin/bash

true=`grep ^stop=true nconf | wc -l`
false=`grep ^stop=false nconf | wc -l`

if [ $true -eq 0 ]
then
	echo 'false -> true'
	if [ $false -eq 0 ]
	then
		echo 'stop=true' >> nconf
	else
		sed s/^stop=false/stop=true/ nconf -i
	fi
else
	echo 'true -> false'
	sed s/^stop=true/stop=false/ nconf -i
fi
