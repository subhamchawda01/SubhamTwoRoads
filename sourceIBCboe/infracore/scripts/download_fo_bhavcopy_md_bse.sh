#!/bin/bash 
# Run this from 16::00 to next day 	2:30
#not safe to do download other files, separately 
#since most of the files generate from those
#better to rerun fetch nse 

download_from_website() {
  wget -t 1 --timeout=20 --referer https://www.bseindia.com --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www.bseindia.com/download/Bhavcopy/Derivative/bhavcopy${DD}-${MM}-${YY}.zip
  unzip -o bhavcopy${DD}-${MM}-${YY}.zip ;
  rm bhavcopy${DD}-${MM}-${YY}.zip;
}

GetPreviousWorkingDay() {
  YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P A`
  is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  while [ $is_holiday_ = "1" ];
  do
    YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P A`
    is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  done
}

YYYYMMDD=`date +"%Y%m%d"`
HHMM=`date +"%H%M"`
if [ ${HHMM} -lt 1000 ];
then
  GetPreviousWorkingDay;
fi
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
if [ $is_holiday = "1" ] ; then
   echo "BSE Holiday. Exiting...";
      exit;
fi
DD=${YYYYMMDD:6:2}
MM=${YYYYMMDD:4:2}
YY=${YYYYMMDD:2:2}
YYYY=${YYYYMMDD:0:4};
MSTR=$(echo $(date -d $YYYYMMDD +%b) | awk '{print toupper($1)}')
BhavCopyDir="/spare/local/tradeinfo/BSE_Files/BhavCopy/fo/"${MM}${YY}
#check whether bhavcopy is already downloaded
mkdir -p "/spare/local/tradeinfo/BSE_Files/BhavCopy/fo/$MM$YY/"
cd ${BhavCopyDir}
[[ $1 == "FORCE" ]] && `rm -rf "bhavcopy${DD}-${MM}-${YY}.csv"`
[ -f "bhavcopy${DD}-${MM}-${YY}.csv" ]  && exit;
download_from_website;
#bhavcopy is not available now, should be available in next run
[ -f "bhavcopy${DD}-${MM}-${YY}.csv" ] || exit;
cat bhavcopy${DD}-${MM}-${YY}.csv | tail -n +2 | awk -F"," '{ OFS=","; print "NORMAL",$1,$2,toupper($3),$4,$5,"-",$6,$7,$8,$9,$14,$15,$16,$17}' > ${DD}${MM}fo_0000.md
chgrp infra bhavcopy${DD}-${MM}-${YY}.csv 
chgrp infra ${DD}${MM}fo_0000.md

#bhavcopy is is updated, send an alert with timestamp sync to all prod machines
TIMESTAMP=`date +"%Y-%d-%d %H:%M:%S"`
[ -f "bhavcopy${DD}-${MM}-${YY}.csv" ] && [ -f "${DD}${MM}fo_0000.md" ] && echo "" | mailx -s "BSE FO BHAVCOPY AND MD IS UPDATED : ${TIMESTAMP}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in

