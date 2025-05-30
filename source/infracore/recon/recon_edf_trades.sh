#!/bin/bash

USAGE="$0 YYYYMMDD";

if [ $# -ne 1 ] ;
then
    echo -e $USAGE;
    exit;
fi

GET_BROKER_TRADES_IN_ORS_FORMAT_SCRIPT=$HOME/infracore/recon/get_edf_eod_trades_in_ors_format.pl;
PNL_SCRIPT=$HOME/infracore_install/scripts/see_ors_pnl.pl;
PAST_PNL_SCRIPT=$HOME/LiveExec/scripts/get_EOD_pnl.sh;
COMPARE_SCRIPT=$HOME/infracore/recon/compare_broker_ors_eod_pnl.py;
FETCH_SCRIPT=$HOME/infracore/recon/fetch_edf_files.sh

YYYYMMDD=$1;
if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
elif [ $YYYYMMDD = "YESTERDAY" ] ;
then
    YYYYMMDD=$(cat /tmp/YESTERDAY_DATE)
fi

#fetch the broker file
$FETCH_SCRIPT $YYYYMMDD

cd /apps/data/EDFTrades/EDFFiles;
IST_FILE="PFDFST4_$YYYYMMDD.CSV";
BROKER_FILE_DIR_PREFIX="/apps/data/EDFTrades";
BROKER_RAW_FILE="$BROKER_FILE_DIR_PREFIX/EDFFiles/$IST_FILE";

BROKER_PNL_FILE_DIR="$BROKER_FILE_DIR_PREFIX/Pnl/"${YYYYMMDD:0:4}"/"${YYYYMMDD:4:2}"/"${YYYYMMDD:6:2};
ORS_PNL_DIR="$HOME/recon";

mkdir -p $BROKER_PNL_FILE_DIR;
mkdir -p $ORS_PNL_DIR;

BROKER_PNL_FILE=$BROKER_PNL_FILE_DIR"/edf_pnl";
BROKER_TRADES_FILE=$BROKER_PNL_FILE_DIR"/edf_trades";
ORS_PNL_FILE=$ORS_PNL_DIR"/edf_ors_pnl";
RESULT_FILE=$ORS_PNL_DIR"/edf_result";
ERROR_FILE=$ORS_PNL_DIR"/edf_err_file";

PNL_RECON_RESULT="/home/dvcinfra/recon/pnl_recon_result"

>$ERROR_FILE
> $RESULT_FILE

if [ ! -f $BROKER_RAW_FILE ] ; then
    echo "<tr bgcolor='#FF0000'><td colspan=6>File not fetched:  $BROKER_RAW_FILE</td></tr>" >> $PNL_RECON_RESULT
    echo "<tr bgcolor='#FF0000'><td colspan=6>File not fetched:  $BROKER_RAW_FILE</td></tr>" >> $RESULT_FILE
    exit 0;
fi

# CME ICE CFE#
$PAST_PNL_SCRIPT $YYYYMMDD > $ORS_PNL_FILE
perl $GET_BROKER_TRADES_IN_ORS_FORMAT_SCRIPT $YYYYMMDD > $BROKER_TRADES_FILE 2>>$ERROR_FILE
sed -i 's/~/ /g' $BROKER_TRADES_FILE
perl $PNL_SCRIPT "R" $BROKER_TRADES_FILE $YYYYMMDD > $BROKER_PNL_FILE 2>>$ERROR_FILE

python $COMPARE_SCRIPT $BROKER_PNL_FILE $ORS_PNL_FILE "Edf" $YYYYMMDD>> $RESULT_FILE

cat $ERROR_FILE | grep RECON >> $RESULT_FILE
if [ -s $RESULT_FILE ]
then
    echo >> $RESULT_FILE
    cat $RESULT_FILE >> $PNL_RECON_RESULT
    echo "<tr bgcolor='#FFFFFF'><td colspan=6>&nbsp;</td></tr>" >> $PNL_RECON_RESULT
fi
