#!/bin/bash


USAGE="$0 [cnt=1] [reconcile=0(NO)]"
cnt=1;
reconcile=0;
mds_fl=/spare/local/files/mds_data_indx_ALL
if [ $# -gt 0 ]; then cnt=$1; fi
if [ $# -gt 1 ]; then reconcile=$2; fi
for i in `seq 1 $cnt`; 
do
    dt="`date -d"-$i day" +%Y%m%d`"
    week_day=`date -d"-$i day" +%u`;  # if week ends;
    if [ "$week_day" -ge "6" ]; then echo "$dt; week-end"; continue; fi; # Sunday
    
    echo $dt;
    a[0]="$dt:0 ";a[1]="$dt:1 ";
    ShortCodeList=`grep contract_specification_map_ /home/dvctrader/dvccode/CDefCode/security_definitions.cpp | sed 's/.*\[ \(.*\) \].*$/\1/g' | sed 's/"//g' | grep -v "HYB_" | sort -u`
    PortCodeList=`cut -d' ' -f2  /spare/local/tradeinfo/PCAInfo/portfolio_inputs_sorted | sort -u`;
    total_list="$ShortCodeList $PortCodeList"
    if [ "$reconcile" -eq "1" ]; then 
	a[1]=`grep "$dt:1" $mds_fl`;  
	total_list=`grep "$dt:0" $mds_fl | cut -d' ' -f2-`
    fi
    if [ -z "$total_list" ]; then 
	total_list="$ShortCodeList $PortCodeList"
    fi;
    for sc in $total_list; do 
	t=`/home/dvctrader/basetrade_install/bin/check_indicator_data $sc $dt 2>/dev/null`
	a[$t]="${a[$t]} $sc"
    done
    b=(${a[0]})
    if [ ${#b[*]} -gt 50 ];
    then 
	echo "A lot of products' data missing. You might want to look once for Date: $dt $mds_fl"
	echo "DATA NOT PRESENT FOR: "
	echo ${a[0]}
    fi
    b=(${a[1]})
    if [ ${#b[*]} -gt 10 ] # otherwise either a holiday or something really gone wrong
    then
	sed -i "/$dt/d" $mds_fl
	echo ${a[0]} >> $mds_fl
	echo ${a[1]} >> $mds_fl
    fi
done

sort -t: -k1 -n $mds_fl -o $mds_fl
$HOME/basetrade/scripts/sync_file_to_all_machines.pl $mds_fl 2>/dev/null 1>&2
