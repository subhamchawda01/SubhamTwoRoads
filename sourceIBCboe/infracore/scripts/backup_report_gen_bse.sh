#!/bin/bash
destpath="/var/www/html/BackupReport/"
wdir="/home/dvcinfra/important/backup_report/"

GetPreviousWorkingDay() {
  YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P A`
  is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  while [ $is_holiday_ = "1" ];
  do
    YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P A`
    is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  done
}

reportindb11(){
 echo "<tr><td>INDB11</td><td>BSELoggedData</td>" >> $REPORTING_FILE ;
  for date_ in ${dates[@]}; do
      yy=`date -d $date_ +"%Y"`
      mm=`date -d $date_ +"%m"`
      dd=`date -d $date_ +"%d"`
      dir_count=`ssh dvcinfra@192.168.132.11 "ls /spare/local/MDSlogs/$yy/$mm/$dd | wc -l"`
      echo "<td>$dir_count</td>" >>$REPORTING_FILE;
  done
  echo "</tr>" >> $REPORTING_FILE ;
  echo "<tr><td>INDB11</td><td>ORSBCAST</td>" >> $REPORTING_FILE ;
  for date_ in ${dates[@]}; do
      yy=`date -d $date_ +"%Y"`
      mm=`date -d $date_ +"%m"`
      dd=`date -d $date_ +"%d"`
      dir_count=`ssh dvcinfra@192.168.132.11 "ls /spare/local/ORSBCAST/BSE/*$yy$mm$dd* | wc -l"`
      echo "<td>$dir_count</td>" >>$REPORTING_FILE;
  done
  echo "</tr>" >> $REPORTING_FILE ;
  echo "<tr><td>INDB11</td><td>BSE_ORSBCAST_MULTISHM</td>" >> $REPORTING_FILE ;
  for date_ in ${dates[@]}; do
      yy=`date -d $date_ +"%Y"`
      mm=`date -d $date_ +"%m"`
      dd=`date -d $date_ +"%d"`
      dir_count=`ssh dvcinfra@192.168.132.11 "ls /spare/local/ORSBCAST_MULTISHM/BSE/*$yy$mm$dd* | wc -l"`
      echo "<td>$dir_count</td>" >>$REPORTING_FILE;
  done
  echo "</tr>" >> $REPORTING_FILE ;
}

report62(){
  echo "<tr><td>62</td><td>BSE_ORSLoggedData</td>" >> $REPORTING_FILE ;
  for date_ in ${dates[@]}; do 
      yy=`date -d $date_ +"%Y"`
      mm=`date -d $date_ +"%m"`
      dd=`date -d $date_ +"%d"`
      dir_count=`ssh root@10.23.5.62 "ls /NAS1/data/ORSData/BSE/$yy/$mm/$dd | wc -l"`
      echo "<td>$dir_count</td>" >>$REPORTING_FILE;
  done
  echo "</tr>" >> $REPORTING_FILE ;
  echo "<tr><td>62</td><td>BSR_ORSBCAST_MULTISHM</td>" >> $REPORTING_FILE ;
  for date_ in ${dates[@]}; do
      yy=`date -d $date_ +"%Y"`
      mm=`date -d $date_ +"%m"`
      dd=`date -d $date_ +"%d"`
      dir_count=`ssh root@10.23.5.62 "ls /NAS1/data/ORSData/ORSBCAST_MULTISHM/BSE/$yy/$mm/$dd | wc -l"`
      echo "<td>$dir_count</td>" >>$REPORTING_FILE;
  done
  echo "</tr>" >> $REPORTING_FILE ;
}

report62(){
  echo "<tr><td>62</td><td>BSELoggedData</td>" >> $REPORTING_FILE ;
  for date_ in ${dates[@]}; do
      yy=`date -d $date_ +"%Y"`
      mm=`date -d $date_ +"%m"`
      dd=`date -d $date_ +"%d"`
      dir_count=`ssh dvcinfra@10.23.5.62 "ls /NAS1/data/BSELoggedData/BSE/$yy/$mm/$dd | wc -l"`
      echo "<td>$dir_count</td>" >>$REPORTING_FILE;
  done
  echo "</tr>" >> $REPORTING_FILE ;
}

report22(){
  echo "<tr><td>22</td><td>BSELoggedData</td>" >> $REPORTING_FILE ;
  for date_ in ${dates[@]}; do
      yy=`date -d $date_ +"%Y"`
      mm=`date -d $date_ +"%m"`
      dd=`date -d $date_ +"%d"`
      dir_count=`ssh dvcinfra@10.23.5.22 "ls /NAS1/data/BSELoggedData/BSE/$yy/$mm/$dd | wc -l"`
      echo "<td>$dir_count</td>" >>$REPORTING_FILE;
  done
  echo "</tr>" >> $REPORTING_FILE ;
}

reportWorker1(){
  echo "<tr><td>Worker1</td><td>BSELoggedData</td>" >> $REPORTING_FILE ;
  for date_ in ${dates[@]}; do
      yy=`date -d $date_ +"%Y"`
      mm=`date -d $date_ +"%m"`
      dd=`date -d $date_ +"%d"`
      dir_count=`ssh dvctrader@54.90.155.232 "ls /NAS5/data/BSELoggedData/BSE/$yy/$mm/$dd | wc -l"`
      echo "<td>$dir_count</td>" >>$REPORTING_FILE;
  done
  echo "</tr>" >> $REPORTING_FILE ;
}

init(){
  YYYYMMDD=`date +"%Y%m%d"`
  declare -a dates

  for (( i= 0;i<5;i++ )); do
      GetPreviousWorkingDay
      dates[$i]=$YYYYMMDD
  done
  echo ${dates[@]}
  mkdir -p $destpath ;
  REPORTING_FILE=$destpath"index.html" ;
  >$REPORTING_FILE ;
  cat $wdir"backup_report_header.txt" | sed "s/time/`date +"%T"` /g" > $REPORTING_FILE ;
  echo "<th>Machine<th>dir</th>" >> $REPORTING_FILE;
  for date_ in ${dates[@]}; do 
      echo "<th>$date_</th>">> $REPORTING_FILE;
  done
  echo "</tr></thead><tbody>" >> $REPORTING_FILE ;
  echo "Computing 62"
  report62
  echo "Computing INDB11"
  reportindb11
  echo "Computing 62"
  report62
  echo "Computing 22"
  report22
  echo "Computing worker"
  reportWorker1
  cat $wdir"backup_report_footer.txt" >> $REPORTING_FILE;
}

init $*


