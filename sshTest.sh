#!/bin/bash

# ホスト名, ログイン名
. hosts

# クライアントの総数
cnum=${#CLIENT[*]}

for (( i = 0 ; i < $cnum ; i++ ))
do
	ssh ${USER[i]}@${CLIENT[i]} echo ${CLIENT[i]} is Ok!
done

ssh ${user}@${server} echo ${server} is Ok!
