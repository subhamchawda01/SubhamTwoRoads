DATE=`date +"%Y%m%d"`
REPORT_FILE=/tmp/open_pos_report.txt
PNL_FILE='/NAS1/data/MFGlobalTrades/ind_pnls/FO/eod_pnls/ind_pnl_'$DATE'.txt'

rm -rf $REPORT_FILE
cat $PNL_FILE | grep NO_OF_LOT | awk '{if($29>=50 || $29 <= -50){print $2"\t"$29}}'  > $REPORT_FILE

if [ -f $REPORT_FILE ] && [ -s $REPORT_FILE ];
then
  cat $REPORT_FILE | mailx -s "ALERT : EOD POS - $DATE " -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" sanjeev.kumar@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in naman.jain@tworoads-trading.co.in tarun.joshi@tworoads-trading.co.in #nseall@tworoads.co.in 
fi
