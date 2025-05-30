#!/bin/bash

MDSLogData() {
  echo "$local66_mds_log_exec NSE $local66_mds_input_file $start_time $end_time"
  ssh dvcinfra@10.23.5.66 "$local66_mds_log_exec NSE $local66_mds_input_file $start_time $end_time" > $local66_mds_output_file
#scp dvcinfra@10.23.5.66:${local66_mds_output_file} ${local66_mds_output_file}
  mds_input_file_count=`wc -l $local66_mds_input_file | cut -d' ' -f1`
  mds_output_file_count=`wc -l $local66_mds_output_file | cut -d' ' -f1`
}

ErrorMail(){
       (echo To: "raghunandan.sharma@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in" ; echo From: "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>"; echo Subject: "FAILED Daily ORS Report for CM Products Generation $today_"; echo "Content-Type: text/html;";echo " ") | /usr/sbin/sendmail -t
#      (echo To: "subham.chawda@tworoads-trading.co.in" ; echo From: "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>"; echo Subject: "FAILED Daily ORS Report for CM Products $today_"; echo "Content-Type: text/html;";echo " ") | /usr/sbin/sendmail -t
       >$error_file_check
       exit;
}

if [ $# -ne 1 ] ; then
  echo "Called As : " $* ;
  echo "$0 Date(20191226)" ;
  exit ;
fi
date_=$1
today_=`date +\%Y\%m\%d`
YYYY=${date_:0:4}
MM=${date_:4:2}
DD=${date_:6:2}

Data_Dir="/NAS1/data/ORSData/ORSBCAST_MULTISHM"
#Data_Dir="/NAS1/data/ORSData/NSE"
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date_ T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi
if [ ! -d "$Data_Dir/$YYYY/$MM/$DD" ]; then
  echo "Data Not copied from IND13";
  echo ""| mailx -s "ORS Data Not found to Gen Html" -r "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>" subham.chawda@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
#  echo ""| mailx -s "ORS Data Not found to Gen Html" -r "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>" subham.chawda@tworoads-trading.co.in 
  exit
fi

Mds_Log="/home/pengine/prod/live_execs/mds_fast_first_trade_read_Volume_Machince_cm"
work_dir='/home/dvcinfra/important/Generate_ors_detail/Product_Details_CM/'
mds_input_file="/home/dvcinfra/trash/ors_reply_mds_cm_input_file_${date_}"
mds_output_file="/home/dvcinfra/important/Generate_ors_detail/REPLY_NSE_OUTPUT_CM/ors_reply_mds_cm_output_file_${date_}"
tmp_file1="/home/dvcinfra/trash/shortcode_cm_file_${date_}"
error_file_check="/home/dvcinfra/trash/error_ors_report_cm"
OutFileCM="$work_dir/${date_}_cm"

rm -rf $error_file_check
mkdir -p $work_dir
>$OutFileCM
>$mds_input_file
for file in $Data_Dir/$YYYY/$MM/$DD/NSE_*; do
    d="$(basename -- $file)";
    echo $file >> $mds_input_file
done

start_time=`date -d"$date_ -1 day" +"%s"`
end_time=`date -d"$date_ +1 day" +"%s"`
echo "$Mds_Log ORS_REPLY_LIVE $mds_input_file $start_time $end_time >$mds_output_file" 
$Mds_Log ORS_REPLY_LIVE $mds_input_file $start_time $end_time >$mds_output_file
echo "MDS Done"
#cp $mds_output_file $OutFileCM
grep -v "Unknown Machine" $mds_output_file | awk '{print $1}' > $tmp_file1
echo "exchange symbol file generated $tmp_file1"

  local66_NSE_FILE_DIR="/NAS1/data/NSELoggedData/NSE/${YYYY}/${MM}/${DD}/"
#local66_NSE_FILE_DIR="/run/media/dvcinfra/NSE_MTBT_2022_06_08/data/NSELoggedData/NSE/${YYYY}/${MM}/${DD}/"
  local66_mds_input_file="/home/dvcinfra/trash/nse_file_list_cm_${date_}"
  local66_mds_output_file="/home/dvcinfra/important/Generate_ors_detail/REPLY_NSE_OUTPUT_CM/mds_nse_cm_output_file_${date_}"
  local66_mds_log_exec="/home/dvcinfra/important/mds_fast_first_trade_read_Volume_Machince_cm"
  awk -v dt=${date_} -v dir=${local66_NSE_FILE_DIR} '{print dir$1"_"dt".gz"}' $tmp_file1 >$local66_mds_input_file
  scp $local66_mds_input_file dvcinfra@10.23.5.66:/home/dvcinfra/trash/
  echo "scp local66_mds_input_file done"
  MDSLogData;
  count=0;
  echo "input_file_count: $mds_input_file_count mds_output_file_count: $mds_output_file_count"
  while [ $mds_input_file_count -ne $mds_output_file_count ]
  do
    echo "count: $count"
    echo "input_file_count: $mds_input_file_count mds_output_file_count: $mds_output_file_count"
    process_check=`ssh dvcinfra@10.23.5.66 "ps aux | grep copy_options_data_part2 | grep -v grep | wc -l"`
    if [[ $process_check -ne 0 ]] 
    then
      sleep 5m
      MDSLogData;
    else
      echo "Differenct Count"
      echo "input_file_count: $mds_input_file_count mds_output_file_count: $mds_output_file_count"
      if [[ $mds_output_file_count -eq 0 ]] 
      then
        ErrorMail
      fi
      break
    fi
    ((++count))
  done
  echo "local66_mds_log_exec done"
  echo "mds log exec excution done on local66 ${local66_mds_output_file}"

awk 'NR==FNR {sym[$1]=$2" "$3" "$4" "$5" "$6; next} {if ($1 in sym) {print $0" "sym[$1]}}' $local66_mds_output_file $mds_output_file >$OutFileCM
echo "MDS ORSREPLY + MDS NSE DATA DONE"
echo "file_path $OutFileCM"
echo ""| mailx -s "CM `cat $OutFileCM|wc -l`" -r "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>" subham.chawda@tworoads-trading.co.in #raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in
