#!/bin/bash

if [ $# -lt 2 ]; then echo "usage: "$0" <work_dir_containing_config_file> <new-stddev>"; exit 1 ; fi

work_dir=$1
std_to_add=$2
cd $work_dir;

TARGET_STD_TXT="TARGET_STDEV_MODEL __-1"__$2"__";

for f in ` grep -H INDICATORLISTFILENAME  * | awk -F ":" '{print $1}' | sort | uniq `
do
ln=`grep -n TARGET_STDEV_MODEL $f | awk -F ":" '{print $1+1}' ` # we expect the first argument to
#targetstdmodel as -1, hence skip that line
if [ $ln ]; then 
	found=`	sed -ne '/TARGET_STDEV_MODEL/,/_/ p'  $f  | grep -v "#" | while read a ; do if [ "$2" == "$a" ] ; then echo "found"  ; fi ; done | wc -l `
	if [ "$found" -eq "0" ] ; then 
		sed -i $ln' a '$2 $f ;
	fi
else 
	sed -i '/DELETE_INTERMEDIATE_/i '"$TARGET_STD_TXT" $f 
	sed -i '/TARGET/s/__/\n/g' $f 
fi

done
cd -


