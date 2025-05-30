#!/bin/bash

USAGE="$0 YYYYMMDD";

if [ $# -ne 1 ] ;
then
    echo -e $USAGE;
    exit;
fi

GET_BROKER_TRADES_IN_ORS_FORMAT_SCRIPT=$HOME/infracore/recon/get_asx_eod_trades_in_ors_format.pl;
PNL_SCRIPT=$HOME/infracore_install/scripts/see_ors_pnl.pl;
PAST_PNL_SCRIPT=$HOME/LiveExec/scripts/get_EOD_pnl.sh;
COMPARE_SCRIPT=$HOME/infracore/recon/compare_broker_ors_eod_pnl.py;
FETCH_SCRIPT=$HOME/infracore/recon/fetch_abnamro_files.sh

YYYYMMDD=$1;
if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
elif [ $YYYYMMDD = "YESTERDAY" ] ;
then
    YYYYMMDD=$(cat /tmp/YESTERDAY_DATE)
fi

#fetch the broker file
chmod +x $FETCH_SCRIPT
$FETCH_SCRIPT $YYYYMMDD

cd /apps/data/AbnamroTrades/AbnamroFiles;
IST_FILE=$YYYYMMDD"_abnamro_trx.csv";
BROKER_FILE_DIR_PREFIX="/apps/data/AbnamroTrades"
BROKER_RAW_FILE="$BROKER_FILE_DIR_PREFIX/AbnamroFiles/$IST_FILE";

BROKER_PNL_FILE_DIR="$BROKER_FILE_DIR_PREFIX/Pnl/"${YYYYMMDD:0:4}"/"${YYYYMMDD:4:2}"/"${YYYYMMDD:6:2};
ORS_PNL_DIR="$HOME/recon";

mkdir -p $BROKER_PNL_FILE_DIR;
mkdir -p $ORS_PNL_DIR;

BROKER_PNL_FILE=$BROKER_PNL_FILE_DIR"/abnamro_pnl";
BROKER_TRADES_FILE=$BROKER_PNL_FILE_DIR"/abnamro_trades";

ORS_PNL_FILE=$ORS_PNL_DIR"/abnamro_ors_pnl";
RESULT_FILE=$ORS_PNL_DIR"/abnamro_result";
ERROR_FILE=$ORS_PNL_DIR"/abnamro_err_file";
ASX_ORS_TRADE_FILE=$ORS_PNL_DIR"/asx_ors_trade_"$YYYYMMDD;
EACH_FILE=$ORS_PNL_DIR"/asx_temp_file";

PNL_RECON_RESULT="/home/dvcinfra/recon/pnl_recon_result"
>$ERROR_FILE
>$RESULT_FILE

if [ ! -f $BROKER_RAW_FILE ] ; then
    echo "<tr bgcolor='#FF0000'><td colspan=6>File not fetched:  $BROKER_RAW_FILE</td></tr>" >> $PNL_RECON_RESULT
    echo "<tr bgcolor='#FF0000'><td colspan=6>File not fetched:  $BROKER_RAW_FILE</td></tr>" >> $RESULT_FILE
    exit 0;
fi

DAY_OF_WEEK=`date -d $YYYYMMDD +%w`
if [ $DAY_OF_WEEK == 1 ] ; then
  LOOK_BACK=3
else
  LOOK_BACK=1
fi

LAST_DATE=`date -d "$YYYYMMDD $LOOK_BACK day ago" +'%Y%m%d'`;
ASX_SERVER="10.23.43.51";
YYYY_TRADE="find /spare/local/ORSlogs/ -name 'trades.$YYYYMMDD' | grep -v DC | grep -v FFDVC | grep -v DBRP | grep -v EJG9";
PREV_YYYY_TRADE="find /spare/local/ORSlogs/ -name 'trades.$LAST_DATE' | grep -v DC | grep -v FFDVC | grep -v DBRP | grep -v EJG9";

first_session_start_=`date -d "$LAST_DATE 22:32" +"%s"`; #GMT
first_session_end_=`date -d "$YYYYMMDD 06:30" +"%s"`; #GMT;
second_session_start_=`date -d "$LAST_DATE 07:12" +"%s"`; #GMT
second_session_end_=`date -d "$LAST_DATE 21:00" +"%s"`;

# get ASX ORS trade #
#get trades from current day's session 1
file=`ssh -o ConnectTimeout=20 dvcinfra@$ASX_SERVER $YYYY_TRADE | grep trades`
>$EACH_FILE
>$ASX_ORS_TRADE_FILE
rsync -avz --timeout=30 --quiet dvcinfra@$ASX_SERVER:$file $EACH_FILE
cat $EACH_FILE | tr '\001' ' ' | awk -v first_session_start_="$first_session_start_" -v first_session_end_="$first_session_end_"  \
	'{if($7>=first_session_start_ && $7<=first_session_end_) print $0}' | tr ' ' '\001' >> $ASX_ORS_TRADE_FILE

#get trades for previous day's session2
file=`ssh -o ConnectTimeout=20 dvcinfra@$ASX_SERVER $PREV_YYYY_TRADE | grep trades`
>$EACH_FILE
rsync -avz --timeout=30 --quiet dvcinfra@$ASX_SERVER:$file $EACH_FILE
cat $EACH_FILE | tr '\001' ' ' | awk -v second_session_start_="$second_session_start_" -v second_session_end_="$second_session_end_" \
      '{if($7>=second_session_start_ && $7<=second_session_end_) print $0}' | tr ' ' '\001' >> $ASX_ORS_TRADE_FILE
perl $PNL_SCRIPT "R" $ASX_ORS_TRADE_FILE $YYYYMMDD > $ORS_PNL_FILE 2>>$ERROR_FILE

#get abnamro trades
perl $GET_BROKER_TRADES_IN_ORS_FORMAT_SCRIPT $YYYYMMDD > $BROKER_TRADES_FILE 2>>$ERROR_FILE
perl $PNL_SCRIPT "R" $BROKER_TRADES_FILE $YYYYMMDD > $BROKER_PNL_FILE 2>>$ERROR_FILE

#compare broker and ors data
python $COMPARE_SCRIPT $BROKER_PNL_FILE $ORS_PNL_FILE "Abnamro" $YYYYMMDD >> $RESULT_FILE

rm -f $ASX_ORS_TRADE_FILE
cat $ERROR_FILE | grep RECON >> $RESULT_FILE
if [ -s $RESULT_FILE ]
then
	echo >> $RESULT_FILE
    cat $RESULT_FILE >> $PNL_RECON_RESULT
    echo "<tr bgcolor='#FFFFFF'><td colspan=6>&nbsp;</td></tr>" >> $PNL_RECON_RESULT
fi
