#!/bin/bash

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
Data_Dir="/spare/local/ORSBCAST_MULTISHM/NSE/"
mds_input_file="/tmp/ors_reply_mds_fo_intraday_input_file"
mds_output_file="/tmp/ors_reply_mds_fo_intraday_output_file"
tmp_file1="/tmp/ors_reply_intraday_temp1"
Mds_Log="/home/pengine/prod/live_execs/mds_fast_first_trade_read_Volume_Machince"
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date_ T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi
#if [ ! -d "$Data_Dir/$YYYY/$MM/$DD" ]; then
#  echo "Data Not copied from IND13";
#  echo ""| mailx -s "ORS Data Not found to Gen Html" -r "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>" subham.chawda@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in
#  exit
#fi


>$mds_input_file
echo "DIR: $Data_Dir"
#for file in `ls ${Data_Dir}* | grep NSE[1-9]* | grep ${date_}`; do
for file in ${Data_Dir}NSE[1-9]*; do
#    d="$(basename -- $file)";
    echo $file >> $mds_input_file
done

grep $date_ $mds_input_file >/tmp/mds_input_temp
mv /tmp/mds_input_temp $mds_input_file

start_time=`date -d"$date_ -1 day" +"%s"`
end_time=`date -d"$date_ +1 day" +"%s"`

echo "$Mds_Log ORS_REPLY_LIVE $mds_input_file $start_time $end_time >$mds_output_file"
$Mds_Log ORS_REPLY_LIVE $mds_input_file $start_time $end_time >$mds_output_file
echo "MDS Done"
#cp $mds_output_file $OutFileFO
scp $mds_output_file dvcinfra@10.23.5.67:/tmp/
#
#echo ""| mailx -s "FO `cat $OutFileFO|wc -l`" -r "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>" subham.chawda@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in
