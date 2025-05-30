#!/bin/bash

Data_Dir="/NAS1/data/ORSData/ORSBCAST_MULTISHM"
#Data_Dir="/NAS1/data/ORSData/NSE"
Mds_Log="/home/pengine/prod/live_execs/mds_fast_first_trade_read_Volume_Machince_fo"
work_dir='/home/dvcinfra/important/Generate_ors_detail/Product_Details_FO/'
mds_input_file="/tmp/ors_reply_mds_fo_input_file"
mds_output_file="/tmp/ors_reply_mds_fo_output_file"
datasource="/spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt"
tmp_file1="/tmp/exch_2_ds_file"
tmp_file2="/tmp/exch_sym_nse_mds_data"
error_file_check="/tmp/error_ors_report_fo"

MDSLogData() {
  echo "$local66_mds_log_exec NSE $local66_mds_input_file $start_time $end_time"
  ssh dvcinfra@10.23.5.66 "$local66_mds_log_exec NSE $local66_mds_input_file $start_time $end_time > $local66_mds_output_file"
  scp dvcinfra@10.23.5.66:${local66_mds_output_file} ${local66_mds_output_file}
  mds_input_file_count=`wc -l $local66_mds_input_file | cut -d' ' -f1`
  mds_output_file_count=`wc -l $local66_mds_output_file | cut -d' ' -f1`
}

ErrorMail(){
#      (echo To: "raghunandan.sharma@tworoads-trading.co.in" ; echo From: "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>"; echo Subject: "FAILED Daily ORS Report for FO Products $date_"; echo "Content-Type: text/html;";echo " ") | /usr/sbin/sendmail -t
       (echo To: "raghunandan.sharma@tworoads-trading.co.in nseall@tworoads.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in" ; echo From: "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>"; echo Subject: "FAILED Daily ORS Report for FO Products Generation $today_"; echo "Content-Type: text/html;";echo " ") | /usr/sbin/sendmail -t
#      (echo To: "subham.chawda@tworoads-trading.co.in" ; echo From: "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>"; echo Subject: "FAILED Daily ORS Report for FO Products $today_"; echo "Content-Type: text/html;";echo " ") | /usr/sbin/sendmail -t
       >$error_file_check
       exit;
}

rm -rf $error_file_check
mkdir -p $work_dir
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
OutFileFO="$work_dir/${date_}_fo"

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date_ T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi
if [ ! -d "$Data_Dir/$YYYY/$MM/$DD" ]; then
  echo "Data Not copied from IND13";
  echo ""| mailx -s "ORS Data Not found to Gen Html" -r "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>" subham.chawda@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in
  exit
fi

>$OutFileFO
>$mds_input_file
for file in $Data_Dir/$YYYY/$MM/$DD/NSE[1-9]*; do
    d="$(basename -- $file)";
    echo $file >> $mds_input_file
done

start_time=`date -d"$date_ -1 day" +"%s"`
end_time=`date -d"$date_ +1 day" +"%s"`
echo "$Mds_Log ORS_REPLY_LIVE $mds_input_file $start_time $end_time >$mds_output_file" 
$Mds_Log ORS_REPLY_LIVE $mds_input_file $start_time $end_time >$mds_output_file
echo "MDS Done"
#cp $mds_output_file $OutFileFO
>$tmp_file1
for prod in `awk '{print $1}' $mds_output_file`; do
  grep $prod $datasource >> $tmp_file1
done
echo "exchange symbol file generated $tmp_file1"

#if [ "$date_" == "$today_" ]; then
#  IND13_NSE_FILE_DIR="/spare/local/MDSlogs/${YYYY}/${MM}/${DD}/"
#  ind13_mds_input_file="/tmp/ind13_nse_file_list"
#  ind13_mds_output_file="/tmp/ind13_mds_nse_output_file"
#  ind13_mds_log_exec="/home/pengine/prod/live_execs/mds_fast_first_trade_read_Volume_Machince"
#  awk '{print $2}' $tmp_file1 | awk -F_ -v dt=${date_} -v dir=${IND13_NSE_FILE_DIR} '{if($3=="CE" || $3=="PE"){ print dir$1"_"$2"_"$3"_"$5"_"$4"_"dt} else {print dir$0"_"dt}}' >$ind13_mds_input_file
#  scp $ind13_mds_input_file dvcinfra@10.23.227.63:/tmp/
#  scp $ind13_mds_input_file dvcinfra@10.23.227.63:/tmp/
#  ssh dvcinfra@10.23.227.63 "$ind13_mds_log_exec NSE $ind13_mds_input_file $start_time $end_time > $ind13_mds_output_file"

#  scp dvcinfra@10.23.227.63:${ind13_mds_output_file} ${ind13_mds_output_file}
#  echo "mds log exec excution done on ind13 ${ind13_mds_output_file}"

#  awk 'NR==FNR {sym[$2]=$1; next} {print sym[$1],$2,$3,$4,$5}' $tmp_file1 $ind13_mds_output_file >$tmp_file2
#else
  local66_NSE_FILE_DIR="/NAS1/data/NSELoggedData/NSE/${YYYY}/${MM}/${DD}/"
  local66_mds_input_file="/tmp/local66_nse_file_list"
  local66_mds_output_file="/tmp/local66_mds_nse_output_file"
  local66_mds_log_exec="/home/dvcinfra/important/mds_fast_first_trade_read_Volume_Machince"
  awk '{print $2}' $tmp_file1 | awk -F_ -v dt=${date_} -v dir=${local66_NSE_FILE_DIR} '{if($3=="CE" || $3=="PE"){ print dir$1"_"$2"_"$3"_"$5"_"$4"_"dt".gz"} else {print dir$0"_"dt".gz"}}' >$local66_mds_input_file
  scp $local66_mds_input_file dvcinfra@10.23.5.66:/tmp/
  echo "scp local66_mds_input_file done"
  MDSLogData;
  count=0;
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
      break
    fi
    ((++count))
  done
  echo "local66_mds_log_exec done"
  echo "mds log exec excution done on local66 ${local66_mds_output_file}"
  awk 'NR==FNR {sym[$2]=$1; next} {if ($1 in sym) {print sym[$1],$2,$3,$4,$5}}' $tmp_file1 $local66_mds_output_file >$tmp_file2
#fi

awk 'NR==FNR {sym[$1]=$2" "$3" "$4" "$5; next} {if ($1 in sym) {print $0" "sym[$1]}}' $tmp_file2 $mds_output_file >$OutFileFO
echo "MDS ORSREPLY + MDS NSE DATA DONE"
echo "file_path $OutFileFO"
echo ""| mailx -s "FO `cat $OutFileFO|wc -l`" -r "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>" subham.chawda@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in
