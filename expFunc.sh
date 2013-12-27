#!/bin/bash
# 棋譜数に応じた損失関数の形状を調べる実験

mkdir ExpFunc

for knum in 1 2 4 8 16
do
	# Number of Records
	echo Number of Records : ${knum}

	# Initialization
	rm evdata
	rm loss
	sed -i -e "s/^directory=.*$/directory=test${knum}/" lconf

	# Optimize a 'evdata'
	./sunfish -l

	mv evinfo ExpFunc/evinfo0_${knum}
	mv evdata ExpFunc/evdata_${knum}

	# Calculate a loss
	for x in 0 50
	do
		cp ExpFunc/evdata_${knum} evdata

		org_v=`./disp king_piece 2578`
		v=`expr ${org_v} + ${x}`

		echo "${org_v} => ${v}"
		./change king_piece 2614 ${v} king_piece 2578 ${v}

		./sunfish -lp
		./sunfish -lg

		loss=`grep "^loss=" learn.info | sed "s/^loss=//"`
		echo "${x} ${loss}" >> loss
	done

	mv loss ExpFunc/loss_${knum}
done
