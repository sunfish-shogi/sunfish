#!/bin/bash

# client.sh
# Client Program for Sunfish

# ホスト名, ログイン名, パスワード
. hosts

# ログファイル
log="client.log"

#clean
rm -rf ~/.sunfish_c

# make directory
mkdir ~/.sunfish_c

# host name
# 全てのクライアントで"違う"名前にしないと駄目!!
hostname=`hostname`
echo $hostname

#resume
resume=0

while :
do
	if [ $resume -eq 0 ]
	then
		# recieve a evdata
		timeout=0
		while [ ! -e ~/.sunfish_c/evdata.cmp -a ! -e ~/.sunfish_c/evdata.cmp0 ]
		do
			sleep 1
		done

		echo "recieved the evdata :" >> $log
		ls ~/.sunfish_c/ >> $log
		date >> $log

		rm evdata
		mv ~/.sunfish_c/evdata.gz ./
		gzip -d evdata.gz

		if [ -e ~/.sunfish_c/evdata.cmp0 ]
		then
			# pvmove
			echo "make a pvmove :" >> $log
			date >> $log

			./retry.sh "./sunfish -lp"
		fi

		rm -f ~/.sunfish_c/evdata.cmp
		rm -f ~/.sunfish_c/evdata.cmp0

		# gradient
		echo "make a gradient.gr :" >> $log
		date >> $log

		rm gradient.gr
		./retry.sh "./sunfish -lg"
	fi

	# send a gradient
	echo "send gradient.gr and learn.info :" >> $log
	date >> $log

	scp gradient.gr $user@$server:~/.sunfish_s/$hostname.gr
	scp learn.info $user@$server:~/.sunfish_s/$hostname.info
	scp client.sh $user@$server:~/.sunfish_s/$hostname.cmp

	resume=0
done
