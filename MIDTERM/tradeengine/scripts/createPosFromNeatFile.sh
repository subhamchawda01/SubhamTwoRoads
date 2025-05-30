#!/bin/bash

file1=$1
file2=$2

for expiry in `cat $file1 $file2 | grep "FUT" | awk -F ',' '{print $3}' | sort -u`; do 
	for prod in `cat $file1 $file2 | grep ",${expiry}," | awk -F ',' '{print $2}' | sort -u`; do
		sum=0;
		for pos in `cat $file1 $file2 | grep ",${expiry}," | grep ",${prod}," | awk -F ',' '{print $12}'`; do
			sum=`echo $sum+$pos | bc -l `;
		done;
		# If condition to dump just non zero positions.
		# remove if not required
		if [ $sum -ne 0 ]
		then
			echo $prod $expiry $sum;
		fi
	done;
done		
