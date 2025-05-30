#!/bin/bash

if [ $# -ne 1 ];then
  echo "DATE(yyyymmdd) argument required,exiting....";
  exit;
fi

DATE=$1
YYYY=${DATE:0:4}
MM=${DATE:4:2}
DD=${DATE:6:2}
DATE_TMP=${DD}"/"${MM}"/"${YYYY};
DATE_DDMMYYYY=${DD}${MM}${YYYY}
BMF11_TRADEFILE=`ssh dvcinfra@10.230.63.11 "find /spare/local/ORSlogs/ -type f -name *trades.$DATE"`
BMF15_TRADEFILE=`ssh dvcinfra@10.230.63.15 "find /spare/local/ORSlogs/ -type f -name *trades.$DATE"`
REPORTS_PATH="/reports/Compliance/BMF/"
REPORT_FILE=${REPORTS_PATH}"AllTradeReports/MOVFUT_DVCAPITAL_$DATE_DDMMYYYY.txt"

if [ "$BMF11_TRADEFILE" == "" ] || [ "$BMF15_TRADEFILE" == "" ];then
    echo "" | mailx -s "Unable to find the BMF trade file" -r sanjeev.kumar@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in
    exit
fi

rsync -avz --progress dvcinfra@10.230.63.11:$BMF11_TRADEFILE ${REPORTS_PATH}"BMF11/"
rsync -avz --progress dvcinfra@10.230.63.15:$BMF15_TRADEFILE ${REPORTS_PATH}"BMF15/"


/apps/anaconda/anaconda3/bin/python /home/pengine/prod/live_scripts/bmf_trade_report_generator.py --date $DATE_TMP --trade_file ${REPORTS_PATH}"BMF11/trades.$DATE" --outfile $REPORT_FILE
/apps/anaconda/anaconda3/bin/python /home/pengine/prod/live_scripts/bmf_trade_report_generator.py --date $DATE_TMP --trade_file ${REPORTS_PATH}"BMF15/trades.$DATE" --outfile $REPORT_FILE 
#send mail
if [ -f $REPORT_FILE ];then
  gzip $REPORT_FILE
  echo "" | mailx -s "BMF TRADE REPORT" -a $REPORT_FILE".gz" -r sanjeev.kumar@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in #ravi.parikh@tworoads.co.in
else
  echo "" | mailx -s "BMF TRADE REPORT : FAILED " -r sanjeev.kumar@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in  # ravi.parikh@tworoads.co.in
  exit
fi

#upload through through ftp
HOST='knox.bmfbovespa.com.br';
USER='dvcapitalllc2_sftp';
KEY='/home/dvcinfra/BMFKEY/dvcapitalllc2_sftp.priv'
sftp -oPort=20681 -oIdentityFile=$KEY $USER@$HOST << SCRIPT
  cd out
  put $REPORT_FILE".gz"
SCRIPT


