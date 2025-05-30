#!/bin/bash

date=`date +%Y%m%d`;
tmp_file="/home/dvctrader/ATHENA/pos_to_exit_${date}_temp"
tmp_file_ind17="/home/dvctrader/ATHENA/pos_to_exit_${date}_temp_ind17"
tmp_file_ind23="/home/dvctrader/ATHENA/pos_to_exit_${date}_temp_ind23"
pos_file="/home/dvctrader/ATHENA/pos_to_exit_${date}"
>$tmp_file
>$tmp_file_ind17
>$tmp_file_ind23

echo "Positions IND17"
for i in `cat  /spare/local/logs/tradelogs/trades.${date}.1238* | awk '{print $3}' | sort -u`; do
	pos=`grep " $i " /spare/local/logs/tradelogs/trades.${date}.1238* | tail -1 | awk '{print $7*-1}'`;
#	exp_=`grep " $i " /spare/local/logs/tradelogs/trades.${date}.1238* | tail -1 | awk '{print sqrt($6*$6*$7*$7)}'`;
	prod=`echo $i | awk -F '.' '{print $1}'`;
	if [ $pos -ne 0 ]; then echo $prod $pos; fi >> $tmp_file_ind17
done

echo "Positions IND23"
mkdir -p /home/dvctrader/trash/IND23_trades/
scp 10.23.227.72:/spare/local/logs/tradelogs/trades.${date}.1239* /home/dvctrader/trash/IND23_trades/

for i in `cat  /home/dvctrader/trash/IND23_trades/trades.${date}.1239* | awk '{print $3}' | sort -u`; do
        pos=`grep " $i " /home/dvctrader/trash/IND23_trades/trades.${date}.1239* | tail -1 | awk '{print $7*-1}'`;
#        exp_=`ssh 10.23.227.72 "grep \" $i \" /spare/local/logs/tradelogs/trades.${date}.1239*" | tail -1 | awk '{print sqrt($6*$6*$7*$7)}'`;
        prod=`echo $i | awk -F '.' '{print $1}'`;
        if [ $pos -ne 0 ]; then echo $prod $pos; fi >> $tmp_file_ind23
done

echo "Computing"
less  $tmp_file_ind17 | sort | awk '{print $1,$2}' > $tmp_file
mv $tmp_file $tmp_file_ind17

less  $tmp_file_ind23 | sort | awk '{print $1,$2}' > $tmp_file
mv $tmp_file $tmp_file_ind23

awk 'ARGV[1] == FILENAME{h[$1] = $2; next};!($1 in h) {print $1,$2}' $tmp_file_ind23 $tmp_file_ind17 > $pos_file
awk 'ARGV[1] == FILENAME{h[$1] = $2; next} {print $1,$2+h[$1]}' $tmp_file_ind17 $tmp_file_ind23 >> $pos_file

rm $tmp_file_ind23 $tmp_file_ind17
