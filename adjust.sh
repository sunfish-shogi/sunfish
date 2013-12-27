#!/bin/bash

#PVを求める。
./sunfish -lp

#待避
mv pvmove0 pvmove0.bak
mv pvmove1 pvmove1.bak
mv pvmove2 pvmove2.bak
mv pvmove3 pvmove3.bak
mv pvmove4 pvmove4.bak
mv pvmove5 pvmove5.bak
mv evdata evdata.bak

mv evinfo evinfo0

#回数を変えて実験
for i in 8 16 32 64 128
do
	#待避したデータを持ってくる。
	rm evdata
	cp pvmove0.bak pvmove0
	cp pvmove1.bak pvmove1
	cp pvmove2.bak pvmove2
	cp pvmove3.bak pvmove3
	cp pvmove4.bak pvmove4
	cp pvmove5.bak pvmove5
	cp evdata.bak evdata

	#i回更新
	j=$i
	while [ $j -ne 0 ]
	do
		./sunfish -lg
		./sunfish -la

		j=`expr $j - 1`
	done

	#損失を計算
	./sunfish -lp
	./sunfish -lg

	#結果を待避
	mv learn.info learn$i.info
done
