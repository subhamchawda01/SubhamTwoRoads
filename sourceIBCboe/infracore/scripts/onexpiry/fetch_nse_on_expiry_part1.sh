#!/bin/bash
#add force will not check for last thursday

FTP_HOST="ftp.connect2nse.com"

check_expiry_date(){
DOW=`date +%u`
nxt_MM=$(date -d "+7 days"+ +%m)
echo $DOW
echo $nxt_MM $MM
if [[ $DOW != 4 ]] || [[ $nxt_MM == $MM ]]; then 
  echo "Not expiry. existing...."
  exit;
fi
}

validate_fo_mktlot_files(){
  month=`date +%b`
  echo ${month^^}
  YY=`date +%y`
  echo $YY
  line1=`head -n 1 fo_mktlots.csv` 
  if [[ $line1 == *"${month^^}-$YY"* ]]; then 
     echo "File not updated.ON FTP "
     rm fo_mktlots.csv
     fetch_mkt_lots_from_web;
     line1=`head -n 1 fo_mktlots.csv`
     if [[ $line1 == *"${month^^}-$YY"* ]]; then
     echo "File not updated.ON WEB also"
     exit -1;
     fi
     echo "LOT FILE File updated ON WEB"
  fi
}

fetch_mkt_lots() {
  ftp -n $FTP_HOST <<SCRIPT
    user "FAOGUEST" "FAOGUEST"
    cd faoftp
    cd faocommon
    binary
    get fo_mktlots.csv
    quit
SCRIPT
}

fetch_mkt_lots_from_web() {
#  wget --referer https://www1.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0"   https://www1.nseindia.com/content/fo/fo_mktlots.csv
  wget --referer https://www.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0"   https://archives.nseindia.com/content/fo/fo_mktlots.csv
  
}

fetch_strike_stream() {
#  wget --referer https://www1.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0"   https://www1.nseindia.com/content/fo/sos_scheme.xls
   wget --referer https://www1.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0"   https://archives.nseindia.com/content/fo/sos_scheme.xls
}


#Main
YYYYMMDD=`date +\%Y\%m\%d`
YYYY=${YYYYMMDD:0:4}
MM=${YYYYMMDD:4:2}
DD=${YYYYMMDD:6:2}

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
[[ $1 == FORCE ]] || check_expiry_date ;
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday...";
   exit;
fi

FTP_HOS='ftp.connect2nse.com'
FTP_DIR='/tmp/NSEFTPFiles/'

rm -rf $FTP_DIR
mkdir -p $FTP_DIR
cd $FTP_DIR
fetch_mkt_lots;
fetch_strike_stream;
if [ ! -f $FTP_DIR/fo_mktlots.csv  ];then
  echo "File not downloaded From FTP. Downloading from web..."
  fetch_mkt_lots_from_web;
fi

if [[ ! -f  $FTP_DIR/fo_mktlots.csv ]] || [[ ! -f $FTP_DIR/sos_scheme.xls ]];then
  echo "Files not downloaded... exiting"
  echo -e "Problem" | mailx -s "Failed fetching lotsize and sos \
       scheme, Need immediate manual intervention. Please take a look asap." \
       -r $HOSTNAME raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in 
  exit
fi
[[ $1 == FORCE ]] || validate_fo_mktlot_files;

#format_mktltos_file
tmp_lot_file=/home/dvctrader/trash/tmp_lot_file.csv
cat fo_mktlots.csv | egrep -v "Derivatives" >> $tmp_lot_file
mv $tmp_lot_file fo_mktlots.csv

EXPIRY=$YYYYMMDD
echo "Expiry : $EXPIRY";
N_MM=$MM
N_YY=$YY
N_YYYY=$YYYY
#if [[ $YYYYMMDD -ge $EXPIRY ]] ; then
   NEXT_MONTH=`date -d "$YYYYMMDD + 1 month - 10 day" +"%m%Y"`
   N_MM=${NEXT_MONTH:0:2}
   N_YY=${NEXT_MONTH:4:2}
   N_YYYY=${NEXT_MONTH:2:4}
echo "NextMonth: $N_MM , $N_YY"
#fi

scp sos_scheme.xls raghu@10.23.5.27:/tmp/sos_scheme.xls
ssh raghu@10.23.5.27 "libreoffice --headless --convert-to csv --outdir /tmp/conv /tmp/sos_scheme.xls"
scp raghu@10.23.5.27:/tmp/conv/sos_scheme.csv $FTP_DIR
cp $FTP_DIR/sos_scheme.csv /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_${N_MM}${N_YY}.csv
cp $FTP_DIR/fo_mktlots.csv /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_${N_MM}${N_YY}.csv

echo "LOT AND SOS FILES UPDATED"
