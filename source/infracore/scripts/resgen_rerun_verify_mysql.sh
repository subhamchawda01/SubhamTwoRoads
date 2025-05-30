#!/bin/bash

USAGE1="$0 date [SHC]"
EXAMP1="$0 20170113"
curr_date=$1
SHC=$2
if [ $# -lt 1 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

if [ -z $SHC ];then
SHC=ALL
echo "empty"
fi


#Check DB for results for shortcodes
echo "~/trash/check_ec2_globalresults_mysql.sh $SHC $curr_date"
~/trash/check_ec2_globalresults_mysql.sh $SHC $curr_date
if [ ! -f "/spare/local/files/mysql_globalresults_status.txt" ]; then 
echo "Running for holiday"
exit 0
fi

zc_file="/spare/local/files/mysql_zero_codes.txt"
if [ -f $zc_file ];then 
rm -f $zc_file
fi

#check for data copy
#ssh -n -f "10.23.74.51" "/home/dvcinfra/scripts/poll_till_datacopy_simple.sh $LOC $curr_date"
#:wqsleep 1000
#start manual run for various sims
lines=`cat /spare/local/files/mysql_globalresults_status.txt`
echo "logs ~/trash/sim_check_run_simulation_log_$curr_date"
echo "logs ~/trash/sim_check_failed_for_log_$curr_date"
echo "clearing old logs RUN for Date: $curr_date" > ~/trash/sim_check_run_simulation_log_$curr_date
echo "clearing old logs RUN for Date: $curr_date" > ~/trash/sim_check_failed_for_log_$curr_date
echo "clearing old logs RUN for Date: $curr_date" > ~/trash/sim_check_run_simulation_errors_$curr_date
while read line; do  
shc=`echo $line|cut -d',' -f1|xargs`
val=`echo $line|cut -d',' -f5|cut -d':' -f2|xargs`;
zc_val=`echo $line|cut -d',' -f6|cut -d':' -f2|xargs`;
zcp=`echo $zc_val'>'50 | bc -l`;
if [ "$zcp" -eq 1 ]; then
echo "$shc , $zcp" >> $zc_file
fi
co=`echo $val'<'100 | bc -l`;  
if [ "$co" -eq 1 ]; then
echo $val
echo $shc 
echo "$shc" >> ~/trash/sim_check_failed_for_log
~/basetrade/ModelScripts/run_simulations.pl $shc "/home/dvctrader/modelling/strats/$shc/" $curr_date $curr_date DB -d 0 -dbg 1 >> ~/trash/sim_check_run_simulation_log_$curr_date 2>>~/trash/sim_check_run_simulation_errors_$curr_date
fi
done <<< "$lines"

echo "zero file availaible at $zc_file"


