#!/bin/bash

EVDATA=evdata
EVINFO=evinfo
BACKUP=Backup

if [ -e ${BACKUP} ]
then
	echo ${BACKUP} is already exists.
else
	echo make directory.. ${BACKUP}
	mkdir ${BACKUP}
fi

while :
do
	num=`grep Loss ${EVINFO} | wc -l`
	num3=`printf %03d ${num}`
	fname=`echo ${EVDATA}_${num3}`
	if [ -e ${BACKUP}/${fname} ]
	then
		echo ${BACKUP}/${fname} is already exists.
	else
		echo backup to.. ${BACKUP}/${fname}
		cp ${EVDATA} ${BACKUP}/${fname}
	fi
	sleep 10000
done
