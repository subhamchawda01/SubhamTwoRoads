#!/bin/bash

Data_Dir="/NAS1/data/ORSData/NSE"
Mds_Log="/home/pengine/prod/live_execs/mds_log_reader"
data_sym='/spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt'
sym_conv='/home/pengine/prod/live_execs/get_shortcode_from_ds'
sym_in_file="/tmp/sym_to_convert"
sym_out_file="/tmp/sym_converted"
#export LD_LIBRARY_PATH=/opt/glibc-2.14/lib ;/home/pengine/prod/live_execs/get_shortcode_from_ds 20191227 FO /tmp/fie /tmp/fil2

if [ $# -ne 1 ] ; then
  echo "Called As : " $* ;
  echo "$0 Date(20191226)" ;
  exit ;
fi

date_=$1
#date_=`date +\%Y\%m\%d`
YYYY=${date_:0:4}
MM=${date_:4:2}
DD=${date_:6:2}

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date_ T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi

if [ ! -d "$Data_Dir/$YYYY/$MM/$DD" ]; then
  echo "Data Not copied from IND13";
  echo ""| mailx -s "ORS Data Not found to Gen Html" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in
  exit
fi

>$sym_in_file
for file in $Data_Dir/$YYYY/$MM/$DD/*; do
    d="$(basename -- $file)";
    [[ $d == *"NSE_"* ]] || { `echo $d |cut -d'_' -f1 >>$sym_in_file`; }         
done

`/home/pengine/prod/live_execs/get_shortcode_from_ds $date_ FO $sym_in_file $sym_out_file`

for file in $Data_Dir/$YYYY/$MM/$DD/*; do
    totcount=`$Mds_Log ORS_REPLY $file | egrep 'SACI: 320|SACI: 314' |egrep -v '\bExec\b|\bConf\b|\bCxRe\b|\bCxld\b' |wc -l`
    d="$(basename -- $file)"
    if [[ $d == *"NSE_"* ]]; then
      sym=`echo $d |cut -d'_' -f2`
    else
       sym_nse=`echo $d|cut -d'_' -f1`
       sym=`grep $sym_nse $sym_out_file | cut -d' ' -f2 | cut -d'_' -f2-`
    fi
    sum=$((sum+totcount))
    echo "$sym $totcount $sum"
done

echo "$date_ $sum"
