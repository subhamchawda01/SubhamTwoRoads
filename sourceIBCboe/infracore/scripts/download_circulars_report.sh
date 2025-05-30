#!/bin/bash 
GetPreviousWorkingDay() {
  fromDate=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P A`
  is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $fromDate T`
  while [ $is_holiday_ = "1" ];
  do
    fromDate=`/home/pengine/prod/live_execs/update_date $fromDate P A`
    is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $fromDate T`
  done
}

dowload_circulars_report() {
	wget --referer https://www1.nseindia.com/ \
	--user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" \
	"https://www.nseindia.com/api/circulars?&fromDate=${fromDate}&toDate=${toDate}&csv=true"
	[ ! -f "circulars?&fromDate=${fromDate}&toDate=${toDate}&csv=true" ] && status=1;
}

filter_circulars_report() {
	grep "Futures & Options" "circulars?&fromDate=${fromDate}&toDate=${toDate}&csv=true"  | egrep "Adjustment" | \
      awk -F "," '{print $5" "$6}' | tr "\"" " "> fo_corporate_events.csv.${YYYYMMDD}
	#can add other things to grep here
	
	rm -rf "circulars?&fromDate=${fromDate}&toDate=${toDate}&csv=true"
}

YYYYMMDD=`date +"%Y%m%d"`
GetPreviousWorkingDay
fromDate=${fromDate:6:2}"-"${fromDate:4:2}"-"${fromDate:0:4}
toDate=`date +"%d-%m-%Y"`
cd /spare/local/tradeinfo/NSE_Files/CorporateEvents
rm -rf "circulars?&fromDate=${fromDate}&toDate=${toDate}&csv=true"
status=0;
dowload_circulars_report
[ $status -ne 0 ] && { echo "" | mailx -s "FAILED DOWNLOADING CIRCULARS REPORT" -r \
		"${HOSTNAME}-${USER}<sanjeev.kumar@tworoads-trading.co.in>" \
        hardik.dhakate@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in \
        raghunandan.sharma@tworoads-trading.co.in; exit; }
filter_circulars_report

if [ -f fo_corporate_events.csv.${YYYYMMDD} ] && [ -s fo_corporate_events.csv.${YYYYMMDD} ];
then
	cat fo_corporate_events.csv.${YYYYMMDD} | mailx -s "FO CORPORATE EVENTS : ${YYYYMMDD}" -r  \
      "${HOSTNAME}-${USER}<sanjeev.kumar@tworoads-trading.co.in>" hardik.dhakate@tworoads-trading.co.in \
      sanjeev.kumar@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in
elif [ ! -s fo_corporate_events.csv.${YYYYMMDD} ];
then
	rm -rf fo_corporate_events.csv.${YYYYMMDD}
fi
