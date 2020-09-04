#!/bin/bash

date=$1
x=$2
cd /home/subham/subham/IND17TradeLogSummary

files=`ls | grep trades.${date}`
result="/tmp/resultroottrade"
>$result
awk '{ if ($9 < -3000) {print $3;}}' trades.20200826.123805 | sort | uniq |wc
for file in $files;
do
	tmp_txt="/tmp/g678"
	echo $file
        awk '{ if ($9 < -3000) {print $3;}}' $file | sort | uniq >$tmp_txt
	while IFS=' ' read -r line
        do
		grep $line $file | tail -1 | awk '{print $3" "$9}' >>$result
	done < $tmp_txt
done
cat $result
awk 'BEGIN{sum =0;pos=0;neg=0} {sum+=$2; if ($2>0)pos++;else neg++;} END{print sum" "pos" "neg}' $result
echo "WRT3000"
awk 'BEGIN{sum =0;pos=0;neg=0} {sum+=$2; if ($2>-3000)pos++;else neg++;} END{print sum" "pos" "neg}' $result
wc $result
