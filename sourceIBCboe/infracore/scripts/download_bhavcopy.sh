#!/bin/bash 
# Run this from 16::00 to next day 	2:30
#not safe to do download other files, separately 
#since most of the files generate from those
#better to rerun fetch nse 
FTP_HOST='ftp.connect2nse.com'

download_from_website() {
	wget --referer https://www1.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www1..nseindia.com/content/historical/DERIVATIVES/"$YYYY"/"$MSTR"/fo"$DD"$MSTR"$YYYY"bhav.csv.zip ;
	unzip -o fo"$DD"$MSTR"$YYYY"bhav.csv.zip ;
}

download_from_ftp() {
ftp -np $FTP_HOST <<SCRIPT 
  user "FAOGUEST" "FAOGUEST" 
  cd faoftp 
  cd faocommon
  cd Bhavcopy 
  binary 
  get fo${DD}${MSTR}${YYYY}bhav.csv.gz
  quit 
SCRIPT
gunzip "fo"$DD$MSTR$YYYY"bhav.csv.gz";
}

GetPreviousWorkingDay() {
  YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P W`
  is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  while [ $is_holiday_ = "1" ];
  do
    YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P W`
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
   echo "NSE Holiday. Exiting...";
      exit;
fi
DD=${YYYYMMDD:6:2}
MM=${YYYYMMDD:4:2}
YY=${YYYYMMDD:2:2}
YYYY=${YYYYMMDD:0:4};
MSTR=$(echo $(date -d $YYYYMMDD +%b) | awk '{print toupper($1)}')
BhavCopyDir="/spare/local/tradeinfo/NSE_Files/BhavCopy/fo/"${MM}${YY}
#check whether bhavcopy is already downloaded
mkdir -p "/spare/local/tradeinfo/NSE_Files/BhavCopy/fo/$MM$YY/"
cd ${BhavCopyDir}
[[ $1 == "FORCE" ]] && `rm -rf "fo"${DD}${MSTR}${YYYY}"bhav.csv"`
[ -f "fo"${DD}${MSTR}${YYYY}"bhav.csv" ]  && exit;
download_from_ftp;
[ ! -f "fo"${DD}${MSTR}${YYYY}"bhav.csv" ] && download_from_website;
#bhavcopy is not available now, should be available in next run
[ -f "fo"${DD}${MSTR}${YYYY}"bhav.csv" ] || exit;
#bhavcopy is is updated, send an alert with timestamp sync to all prod machines
TIMESTAMP=`date +"%Y-%d-%d %H:%M:%S"`
[ -f "fo"${DD}${MSTR}${YYYY}"bhav.csv" ]  && echo "" | mailx -s "BHAVCOPY IS UPDATED : ${TIMESTAMP}" -r $HOSTNAME raghunandan.sharma@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in
