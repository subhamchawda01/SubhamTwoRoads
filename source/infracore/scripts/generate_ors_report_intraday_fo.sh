#!/bin/bash


init () {
  path="/home/dvcinfra/important/ORS_REPLY_INTRADAILY/script/";
  tmp_file1="/tmp/ors_reply_intraday_temp1"
  mds_input_file="/tmp/ors_reply_mds_fo_intraday_input_file"
  mds_output_file="/tmp/ors_reply_mds_fo_intraday_output_file"
  datasource="/spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt"
  Mds_Log="/home/pengine/prod/live_execs/mds_fast_first_trade_read_Volume_Machince"

  date_=$1
  YYYY=${date_:0:4}
  MM=${date_:4:2}
  DD=${date_:6:2}
  echo "Date: $date_ yyyy= $YYYY mm= $MM dd= $DD"
  Data_Dir="/NAS1/data/ORSData/ORSBCAST_MULTISHM/${YYYY}/${MM}/${DD}/"
  >$tmp_file1
  is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date_ T`
  if [ $is_holiday = "1" ] ; then
       echo "NSE Holiday. Exiting...";
     exit;
  fi
  mkdir -p $Data_Dir
#  
  ssh dvcinfra@10.23.227.66 "/home/pengine/prod/live_scripts/rsync_67_fo_ratio.sh"
#  ssh dvcinfra@10.23.227.66 "/home/dvcinfra/important/get_ors_reply_fo_ratio_intraday.sh $date_"
  >$mds_input_file
  echo "DIR: $Data_Dir"
  for file in ${Data_Dir}NSE[1-9]*; do
        echo $file >> $mds_input_file
  done
  mds_input_cnt=`wc $mds_input_file`
  echo "mds_input_cnt=: $mds_input_cnt"
  grep $date_ $mds_input_file >/tmp/mds_input_temp
  mv /tmp/mds_input_temp $mds_input_file

  start_time=`date -d"$date_ -1 day" +"%s"`
  end_time=`date -d"$date_ +1 day" +"%s"`

  echo "$Mds_Log ORS_REPLY_LIVE $mds_input_file $start_time $end_time >$mds_output_file"
  $Mds_Log ORS_REPLY_LIVE $mds_input_file $start_time $end_time >$mds_output_file
  echo "MDS Done"

#
  cnt=`wc $mds_output_file`
  echo "mds_output: $cnt"
  for prod in `awk '{print $1}' $mds_output_file`; do
      grep $prod $datasource >> $tmp_file1
  done
  cnt2=`wc $tmp_file1`
  echo "tmp_file cnt: $cnt2"

  awk 'NR==FNR {sym[$1]=$2; next} {print sym[$1],$18,$19,$20,$21}' $tmp_file1 $mds_output_file >/tmp/intraday_fo_file 
  echo "awk 'NR==FNR {sym[$1]=$2; next} {print sym[\$1],\$18,\$19,\$20,\$21}' $tmp_file1 $mds_output_file >/tmp/intraday_fo_file"
  REPORTING_FILE="/tmp/intraday_trade_ratio_fo.html" ;
  >$REPORTING_FILE ;
  day=`date +"%Y%m%d %H:%M:%S"`;
  cat $path"generate_ors_header.txt" > $REPORTING_FILE ;
  echo "<h3>INTRADAY RATIO ORS REPORTS FO $day</h3></div>" >> $REPORTING_FILE ;
  echo "<table id='myTable' class='table table-striped' ><thead><tr>" >> $REPORTING_FILE
  echo "<th>Symbol</th><th>TotalOrder</th><th>TotalExec</th><th>TotalRatio</th><th>Modify</th><th>ModifyOF</th><th>ModifyOF%</th></tr></thead><tbody>" >> $REPORTING_FILE;
  while IFS=' ' read -r sym f1 f2 f3 f4
  do
        ratio_=$f3
        [[ ${f4} == 0 ]] || ratio_=$((${f3} / ${f4}))

        modify_of_per=0
        [[ ${f2} == 0 ]] || modify_of_per=`echo "$f1 $f2" | awk '{printf "%f", $2*100/$1}'` #modify_of_per=$((${f2} * 100 / ${f1}))
        echo "<tr><td><b>$sym</b></td><td>$f3</td><td>$f4</td><td>$ratio_</td><td>$f1</td><td>$f2</td><td>$modify_of_per</td></tr>" >> $REPORTING_FILE;
  done </tmp/intraday_fo_file

  cat $path"generate_ors_footer.txt" >> $REPORTING_FILE;
  ct=`wc $REPORTING_FILE`
  echo "report cnt: $ct"
  cp $REPORTING_FILE /var/www/html/intraday_trade_ratio_fo/index.html
  chmod +777 /var/www/html/intraday_trade_ratio_fo/index.html
}

init $*
