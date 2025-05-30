#!/bin/bash

Data_Dir="/NAS1/data/ORSData/NSE"
Mds_Log="/tmp/mds_log_reader"
data_sym='/spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt'
sym_conv='/home/pengine/prod/live_execs/get_shortcode_from_ds'
sym_in_file="/tmp/sym_to_convert"
sym_out_file="/tmp/sym_converted"
overall="/tmp/sumall"
mds_input_file="/home/dvcinfra/trash/ors_reply_mds_input_file"
mds_output_file="/home/dvcinfra/trash/ors_reply_mds_output_file"

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

>$mds_input_file
>$sym_in_file
for file in $Data_Dir/$YYYY/$MM/$DD/*; do
    d="$(basename -- $file)";
    echo $file >> $mds_input_file
    [[ $d == *"NSE_"* ]] || { `echo $d |cut -d'_' -f1 >>$sym_in_file`; }         
done
echo "Naming"
`/home/pengine/prod/live_execs/get_shortcode_from_ds $date_ FO $sym_in_file $sym_out_file`
echo "Shortcode Done"
start_time=`date -d"$date_ -1 day" +"%s"`
end_time=`date -d"$date_ +1 day" +"%s"`
$Mds_Log ORS_REPLY $mds_input_file $start_time $end_time >$mds_output_file
echo "MDS Done"
sum=`egrep 'SACI: 320|SACI: 314' $mds_output_file |egrep -v '\bExec\b|\bConf\b|\bCxRe\b|\bCxld\b' | wc -l`
echo "TOTAL: $sum"
echo "$date_ $sum" >>$overall
