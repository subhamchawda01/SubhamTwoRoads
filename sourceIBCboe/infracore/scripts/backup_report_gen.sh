#!/bin/bash
destpath="/var/www/html/BackupReport/"
wdir="/root/backup_report/"

GetPreviousWorkingDay() {
  YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P A`
  is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  while [ $is_holiday_ = "1" ];
  do
    YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P A`
    is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  done
}

report67(){
#  echo "<tr><td>67</td><td>NSELoggedData</td>" >> $REPORTING_FILE ;
#  for date_ in ${dates[@]}; do
#      yy=`date -d $date_ +"%Y"`
#      mm=`date -d $date_ +"%m"`
#      dd=`date -d $date_ +"%d"`
#      dir_count=`ssh root@10.23.5.67 "ls /NAS1/data/NSELoggedData/NSE/$yy/$mm/$dd | wc -l"`
#      echo "<td>$dir_count</td>" >>$REPORTING_FILE;
#  done
#  echo "</tr>" >> $REPORTING_FILE ;
  echo "<tr><td>67</td><td>ORSDATA</td>" >> $REPORTING_FILE ;
  for date_ in ${dates[@]}; do
      yy=`date -d $date_ +"%Y"`
      mm=`date -d $date_ +"%m"`
      dd=`date -d $date_ +"%d"`
      dir_count=`ssh root@10.23.5.67 "ls /NAS1/data/ORSData/NSE/$yy/$mm/$dd | wc -l"`
      echo "<td>$dir_count</td>" >>$REPORTING_FILE;
  done
  echo "</tr>" >> $REPORTING_FILE ;
  echo "<tr><td>67</td><td>Q19</td>" >> $REPORTING_FILE ;
  for date_ in ${dates[@]}; do
      yy=`date -d $date_ +"%Y"`
      mm=`date -d $date_ +"%m"`
      dd=`date -d $date_ +"%d"`
      dir_count=`ssh root@10.23.5.67 "ls /NAS1/data/ORSData/ORSBCAST_MULTISHM/$yy/$mm/$dd | wc -l"`
      echo "<td>$dir_count</td>" >>$REPORTING_FILE;
  done
  echo "</tr>" >> $REPORTING_FILE ;
}

reportind13(){
 echo "<tr><td>IND13</td><td>NSELoggedData</td>" >> $REPORTING_FILE ;
 echo "IND13 GENERICLOGGED"
  for date_ in ${dates[@]}; do
      yy=`date -d $date_ +"%Y"`
      mm=`date -d $date_ +"%m"`
      dd=`date -d $date_ +"%d"`
      dir_count=`ssh dvcinfra@10.23.227.63 "ls /spare/local/MDSlogs/$yy/$mm/$dd | wc -l"`
      echo "<td>$dir_count</td>" >>$REPORTING_FILE;
  done
  echo "</tr>" >> $REPORTING_FILE ;
  echo "IND13 ORSDATA"
  echo "<tr><td>IND13</td><td>ORSDATA</td>" >> $REPORTING_FILE ;
  for date_ in ${dates[@]}; do
      yy=`date -d $date_ +"%Y"`
      mm=`date -d $date_ +"%m"`
      dd=`date -d $date_ +"%d"`
      dir_count=`ssh dvcinfra@10.23.227.63 "ls /spare/local/MDSlogs/ORS_OLD_$yy$mm$dd | wc -l"`
      echo "<td>$dir_count</td>" >>$REPORTING_FILE;
  done
  echo "</tr>" >> $REPORTING_FILE ;
  echo "IND13 ORSCAST"
  echo "<tr><td>IND13</td><td>Q19</td>" >> $REPORTING_FILE ;
  for date_ in ${dates[@]}; do
      yy=`date -d $date_ +"%Y"`
      mm=`date -d $date_ +"%m"`
      dd=`date -d $date_ +"%d"`
      dir_count=`ssh dvcinfra@10.23.227.63 "ls /spare/local/ORSBCAST_MULTISHM/NSE/*$yy$mm$dd* | wc -l"`
      echo "<td>$dir_count</td>" >>$REPORTING_FILE;
  done
  echo "IND13 END"
  echo "</tr>" >> $REPORTING_FILE ;
}

reportworker(){
  echo "<tr><td>Worker</td><td>NSELoggedData</td>" >> $REPORTING_FILE ;
  for date_ in ${dates[@]}; do
      yy=`date -d $date_ +"%Y"`
      mm=`date -d $date_ +"%m"`
      dd=`date -d $date_ +"%d"`
      #dir_count=`ssh dvctrader@3.89.148.73 "ls /NAS1/data/NSELoggedData/NSE/$yy/$mm/$dd | wc -l"`
      dir_count=`ssh dvctrader@54.90.155.232 "ls /NAS1/data/NSELoggedData/NSE/$yy/$mm/$dd | wc -l"`
      echo "<td>$dir_count</td>" >>$REPORTING_FILE;
  done
  echo "</tr>" >> $REPORTING_FILE ;
}

report66(){
  echo "<tr><td>66</td><td>NSELoggedData</td>" >> $REPORTING_FILE ;
  for date_ in ${dates[@]}; do 
      yy=`date -d $date_ +"%Y"`
      mm=`date -d $date_ +"%m"`
      dd=`date -d $date_ +"%d"`
      dir_count=`ssh root@10.23.5.66 "ls /NAS1/data/NSELoggedData/NSE/$yy/$mm/$dd | wc -l"`
      echo "<td>$dir_count</td>" >>$REPORTING_FILE;
  done
  echo "</tr>" >> $REPORTING_FILE ;
}

report42(){
  echo "<tr><td>42</td><td>ORSData</td>" >> $REPORTING_FILE ;
  for date_ in ${dates[@]}; do
      echo "42: date $date_"
      yy=`date -d $date_ +"%Y"`
      mm=`date -d $date_ +"%m"`
      dd=`date -d $date_ +"%d"`
      dir_count=`ls "/NAS1/BACKUP/ORSDATA/NSE/NSE/$yy/$mm/$dd" | wc -l`
      echo "<td>$dir_count</td>" >>$REPORTING_FILE;
  done
  echo "</tr>" >> $REPORTING_FILE ;
  echo "<tr><td>42</td><td>Q19</td>" >> $REPORTING_FILE ;
  for date_ in ${dates[@]}; do
      dir_count=`ls "/NAS1/BACKUP/ORSBCAST_MULTISHM_Q19"| grep $date_ | wc -l`
      echo "<td>$dir_count</td>" >>$REPORTING_FILE;
  done
  echo "ENd 42"
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
  echo "Computing 67"
  report67
  echo "Computing worker"
  reportworker
  echo "Computing 66"
  report66
  echo "Computing 42"
  report42
  echo "Computing IND13"
  reportind13
  cat $wdir"backup_report_footer.txt" >> $REPORTING_FILE;
}

init $*

