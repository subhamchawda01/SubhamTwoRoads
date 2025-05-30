#!/bin/bash

USAGE="$0 YYYYMMDD \n\t >> Generate pnl csvs for date YYYYMMDD.";

if [ $# -ne 1 ] ; 
then 
    echo -e $USAGE;
    exit;
fi

BCS_FORMAT_SCRIPT=$HOME/infracore/recon/get_general_bcs_eod_trades_in_ors_format.pl;
PNL_SCRIPT=$HOME/infracore_install/scripts/see_ors_pnl.pl;
PAST_PNL_SCRIPT=$HOME/LiveExec/scripts/get_EOD_pnl.sh;
COMPARE_SCRIPT=$HOME/infracore/recon/compare_broker_ors_eod_pnl.py;
FETCH_SCRIPT=$HOME/infracore/recon/fetch_bcs_files.sh

YYYYMMDD=$1;
if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
elif [ $YYYYMMDD = "YESTERDAY" ] ;
then
    YYYYMMDD=$(cat /tmp/YESTERDAY_DATE)
fi

$FETCH_SCRIPT $YYYYMMDD

cd /apps/data/BCSTrades/BCSFiles;
BCS_TRD_FILE="/apps/data/BCSTrades/BCSFiles/"$YYYYMMDD"_trades";
BCS_PNL_FILE_DIR="/apps/data/BCSTrades/Pnl/"${YYYYMMDD:0:4}"/"${YYYYMMDD:4:2}"/"${YYYYMMDD:6:2};
ORS_PNL_DIR="$HOME/recon";

mkdir -p $BCS_PNL_FILE_DIR;
mkdir -p $ORS_PNL_DIR;

ORS_PNL_FILE=$ORS_PNL_DIR"/bcs_ors_pnl";
RESULT_FILE=$ORS_PNL_DIR"/bcs_result";
ERROR_FILE=$ORS_PNL_DIR"/bcs_err_file";

BCS_PNL_FILE=$BCS_PNL_FILE_DIR"/bcs_pnl";
BCS_TRADES_FILE=$BCS_PNL_FILE_DIR"/bcs_trades";

PNL_RECON_RESULT="/home/dvcinfra/recon/pnl_recon_result"

>$ERROR_FILE
>$RESULT_FILE

if [ ! -f $BCS_TRD_FILE ] ; then
    echo "<tr bgcolor='#FF0000'><td colspan=6>File not fetched: $BCS_TRD_FILE</td></tr>" >> $PNL_RECON_RESULT
    echo "<tr bgcolor='#FF0000'><td colspan=6>File not fetched:  $BCS_TRD_FILE</td></tr>" >> $RESULT_FILE
    exit 0;
fi

# RTS MICEX # 
$PAST_PNL_SCRIPT $YYYYMMDD > $ORS_PNL_FILE
perl $BCS_FORMAT_SCRIPT $BCS_TRD_FILE $YYYYMMDD > $BCS_TRADES_FILE 2>>$ERROR_FILE
perl $PNL_SCRIPT "R" $BCS_TRADES_FILE $YYYYMMDD > $BCS_PNL_FILE 2>>$ERROR_FILE
python $COMPARE_SCRIPT $BCS_PNL_FILE $ORS_PNL_FILE "Bcs" $YYYYMMDD>> $RESULT_FILE

cat $ERROR_FILE | grep RECON >> $RESULT_FILE
if [ -s $RESULT_FILE ]  
then  
    echo >> $RESULT_FILE
    cat $RESULT_FILE >> $PNL_RECON_RESULT
    echo "<tr bgcolor='#FFFFFF'><td colspan=6>&nbsp;</td></tr>" >> $PNL_RECON_RESULT
fi
