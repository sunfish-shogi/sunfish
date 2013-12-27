#!/bin/bash

# server.sh
# Server Program for Sunfish

# 繰り返し回数
steps=5000

# ホスト名, ログイン名, パスワード
. hosts

# クライアントの総数
cnum=${#CLIENT[*]}

# ログファイル
log="server.log"

# clean
rm -rf ~/.sunfish_s

# make directory
mkdir ~/.sunfish_s

# host name
hostname=`hostname`
echo $hostname

# number of clients
echo "Clients : $cnum" >> $log

# cycle
pv=0
cycle=50
cnt=0
max_cnt=42

for (( i = 0 ; i < $steps ; i++ ))
do
	echo "step : $i"
	echo "step : $i" >> $log
	date >> $log

	if [ $i -eq $pv ]
	then
		num3=`printf %03d ${cnt}`
		echo "backup to evdata_${num3}"
		cp evdata evdata_${num3}
		echo "update PV!!" >> $log
		cmp="evdata.cmp0"
		pv=`expr $pv + $cycle`
		#cycle=`expr \( $cycle \* 9 + 10 - 1 \) / 10`
		cnt=`expr $cnt + 1`

		if [ $cnt -eq $max_cnt ]
		then
			break
		fi
	else
		echo "not update PV." >> $log
		cmp="evdata.cmp"
	fi

	# send evdata
	echo "broadcast a evdata :" >> $log
	date >> $log
	gzip -c evdata > evdata.gz

	for(( j = 0 ; j < $cnum ; j++ ))
	do
		scp evdata.gz ${USER[j]}@${CLIENT[j]}:~/.sunfish_c/
		scp server.sh ${USER[j]}@${CLIENT[j]}:~/.sunfish_c/${cmp}
	done

	echo "broadcasting is completed :" >> $log
	date >> $log

	# recieve gradients
	fnum=`find ~/.sunfish_s -name '*.cmp' | wc -l`
	timeout=0
	while [ $fnum -lt $cnum -a $timeout -lt 30 ]
	do
		sleep 1
		fnum=`find ~/.sunfish_s -name '*.cmp' | wc -l`
		fnum2=`find ~/.sunfish_s -name '*.gr' | wc -l`
		if [ $fnum2 -eq $cnum ]
		then
			timeout=`expr $timeout + 1`
		fi
	done

	echo "gradients : $fnum" >> $log
	ls ~/.sunfish_s/ >> $log

	# adjust
	echo "adjust :" >> $log
	date >> $log
	./retry.sh "./sunfish -la"

	# remove gradients
	rm -f ~/.sunfish_s/*.gr
	rm -f ~/.sunfish_s/*.info
	rm -f ~/.sunfish_s/*.cmp
done
