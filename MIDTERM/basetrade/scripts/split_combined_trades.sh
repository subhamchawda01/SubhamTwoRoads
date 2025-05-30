#!/bin/bash
if [ $# -le 0 ];
then 
    echo "Give list of files to split. New files will be generated in the same directory. 
Original file will be moved to _bak extetion
"
    exit;
fi

trd_fl_name=$*

for trd_fl in $trd_fl_name;
do
    ttrd_fl=${trd_fl_name}_bak
    cp $trd_fl_name $ttrd_fl
    tid=${trd_fl##*.}
    for i in `cut -d' ' -f3  $ttrd_fl | sort -u`; 
    do 
	nid=${i##*.}
	ntrd_fl=${trd_fl//$tid/$nid}
	echo $i $ntrd_fl 
	grep $i $ttrd_fl > $ntrd_fl
    done
#    rm $ttrd_fl
done


