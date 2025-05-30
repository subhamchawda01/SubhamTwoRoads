#!/bin/bash

USAGE="$0 SHORTCODE ILIST END_DATE NUM_DAYS START_TIME END_TIME";

if [ $# -lt 6 ];
then
    echo $USAGE;
    exit;
fi

shortcode=$1;
ilist=$2;
end_date=$3;
num_days=$4;
start_time=$5;
end_time=$6;

temp=`date +%s%N`;
user=`whoami`;
if [ -d "/spare/local/$user" ] ; then
work_dir="/spare/local/$user/res_$temp/" ;
else
mkdir /spare/local/$user ;
work_dir="/spare/local/$user/res_$temp/" ;
fi

num_indicators=`cat $ilist | awk '{if($1=="INDICATOR"){c+=1}}END{print c}'`

/home/dvctrader/basetrade_install/scripts/get_regdata.py $shortcode $ilist $end_date $num_days $start_time $end_time 1000 0 ts1 0 100 na_e3 fsg.5 $work_dir

min_price_increment=`/home/dvctrader/basetrade_install/bin/get_min_price_increment $shortcode $end_date`
num_indicators=`expr $num_indicators + 1`
for i in `seq 2 $num_indicators`
do
    awk -v col=$i '{print sqrt($col^2)}' ${work_dir}/filtered_regdata_filename | sort -n | awk -v mp=$min_price_increment 'BEGIN{c=0}{value[c]=$1;c++;}END{for(i=0.5;i<0.99;i+=0.1){print i, value[int(NR*i)], mp/value[int(NR*i)]}}'
    echo "-------------"
done

rm -rf $work_dir

