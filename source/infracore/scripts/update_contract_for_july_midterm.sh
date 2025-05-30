#!/bin/bash

#Main 
if [ $# -ne 1 ] ; then
  echo "Called As : DATE" ;
  exit
fi



if [ "$1" == "TODAY" ];then
        Date=`date +"%Y%m%d"`;
else
        Date=$1;
fi

next_working_day=`/home/pengine/prod/live_execs/update_date $Date N A`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
while [[ $is_holiday = "1" ]] 
do
    next_working_day=`/home/pengine/prod/live_execs/update_date $next_working_day N A`
    is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
done

Date=$next_working_day

file="/spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.${Date}"
file2="/spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.${Date}_bkp"
tmp_file="/tmp/nse_contracts.${Date}_tmp"
echo "FILE: $file"
>$file2
>$tmp_file

IFS=$'\n'     
for line in `cat $file`;do
  type_=`echo $line| cut -d$'\t' -f1`
  shortcode_=`echo $line| cut -d$'\t' -f2`
  expiry_=`echo $line| cut -d$'\t' -f6`
  if [[ $type_ == "IDXOPT" ]] && [[ $shortcode_ == "NIFTY" ]] && [[ $expiry_ -lt 20210729 ]];then
    echo $line >> $tmp_file
    continue;
  fi
  echo $line >> $file2
done
awk '{$4+=25}1' FS="\t" OFS="\t" $tmp_file >> $file2

diff $file $file2 
mv $file "/spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.${Date}_bkp_old"

mv $file2 $file
