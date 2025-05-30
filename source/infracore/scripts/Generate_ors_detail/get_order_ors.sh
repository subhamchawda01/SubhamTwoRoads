#!/bin/bash

Data_Dir="/NAS1/data/ORSData/NSE"
#Mds_Log="/home/dvcinfra/important/Generate_ors_detail/mds_fast_first_trade_read_Volume_generic_update"
Mds_Log="/home/pengine/prod/live_execs/mds_fast_first_trade_read_Volume_generic_update3"
work_dir='/home/dvcinfra/important/Generate_ors_detail/Product_Details/'
mds_input_file="/tmp/ors_reply_mds_input_file"
mds_output_file="/tmp/ors_reply_mds_output_file"

mkdir -p $work_dir
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
OutFileCash="$work_dir/${date_}_cash"

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date_ T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi
if [ ! -d "$Data_Dir/$YYYY/$MM/$DD" ]; then
  echo "Data Not copied from IND13";
  echo ""| mailx -s "ORS Data Not found to Gen Html" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in
  exit
fi

>$OutFileCash
>$mds_input_file
for file in $Data_Dir/$YYYY/$MM/$DD/NSE_*; do
    d="$(basename -- $file)";
    echo $file >> $mds_input_file
done

start_time=`date -d"$date_ -1 day" +"%s"`
end_time=`date -d"$date_ +1 day" +"%s"`
echo "$Mds_Log ORS_REPLY $mds_input_file $start_time $end_time >$mds_output_file" 
$Mds_Log ORS_REPLY $mds_input_file $start_time $end_time >$mds_output_file
echo "MDS Done"
cp $mds_output_file $OutFileCash

echo ""| mailx -s "Cash `cat $OutFileCash|wc -l`" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in
echo "Updating HTML"
/home/dvcinfra/important/Generate_ors_detail/script/generate_ors_report.sh
