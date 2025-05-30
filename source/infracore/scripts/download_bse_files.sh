#!/bin/bash
declare -A SecNameIndex

GetPreviousWorkingDay() {
  YYYYMMDD=`/home/pengine/prod/live_execs/update_date $date P A`
  is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  while [ $is_holiday_ = "1" ];
  do
    YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P A`
    is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  done
}


work_dir="/spare/local/tradeinfo/NSE_Files/CorporateAnnouncements/"
date=`date +\%Y\%m\%d`
GetPreviousWorkingDay
DD=${YYYYMMDD:6:2}
MM=${YYYYMMDD:4:2}
YYYY=${YYYYMMDD:0:4}

output_data="$work_dir/Announcements_$date"
nse_sec_file="/spare/local/files/NSEFTPFiles/${YYYY}/${MM}/${DD}/security"
touch $output_data

cd $work_dir/Files
if [[ ! -f /tmp/CorporateAnnouncement_${date} ]]; then
    rm scrip.zip
    rm -rf $work_dir/Files/SCRIP
    wget  https://www.bseindia.com/downloads/Help/file/scrip.zip
    unzip scrip.zip
    touch /tmp/CorporateAnnouncement_${date}
#zero entry for prev day file
    lastday_file="$work_dir/Announcements_$YYYYMMDD"
    prev_time=`date -d"$MM/$DD/$YYYY 10:00:00" +"%s"`
    echo $lastday_file $prev_time
    while IFS=' ' read -r f1 f2 f3
    do
        if [[ $prev_time -lt $f3 ]];then
            echo "$f1 $f2 0" >>$output_data     
        fi
    done < $lastday_file
fi

securityFile=`ls "$work_dir/Files/SCRIP" | grep "SCRIP_"`
securityFile="$work_dir/Files/SCRIP/${securityFile}"
echo $securityFile
while IFS='|' read -r f1 f2 f3 line
do
    SecNameIndex[$f1]=$f3
done < $securityFile

out_content="/tmp/all_result_output_bse"
/home/dvctrader/.conda/envs/env/bin/python /home/dvctrader/important/BseEarningAndBroadMeeting/download_broad_time_bse.py >$out_content 2>/dev/null

while IFS=' ' read -r f1 f2 f3
do
  echo ${SecNameIndex[$f1]} $f1
  timestamp=`date -d "$f3 IST" +"%s"`
  echo "grep "${SecNameIndex[$f1]}" $nse_sec_file | grep 'EQ'"
  if grep "|${SecNameIndex[$f1]}|" $nse_sec_file | grep -q "EQ"
  then
      if ! grep -q "NSE_${SecNameIndex[$f1]} $f2 $timestamp" $output_data
      then
          echo "ENTRY"
          echo "NSE_${SecNameIndex[$f1]} $f2 $timestamp" >>$output_data
      fi
  fi

done < $out_content



rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.81:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.82:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.83:/spare/local/tradeinfo --delete-after
#rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.71:/spare/local/tradeinfo --delete-after
