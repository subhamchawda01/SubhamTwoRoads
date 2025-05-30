#!/bin/bash

USAGE="$0 YYYYMMDD \n\t >> Generate pnl csvs for date YYYYMMDD.";

if [ $# -ne 1 ] ; 
then 
    echo -e $USAGE;
    exit;
fi

GENERAL_FORMAT_SCRIPT=$HOME/infracore/recon/get_general_plural_eod_trades_in_ors_format.pl;
PNL_SCRIPT=$HOME/infracore_install/scripts/see_ors_pnl.pl;
PAST_PNL_SCRIPT=$HOME/LiveExec/scripts/get_EOD_pnl.sh;
COMPARE_SCRIPT=$HOME/infracore/recon/compare_broker_ors_eod_pnl.py;
FETCH_SCRIPT=$HOME/infracore/recon/fetch_plural_files.sh

YYYYMMDD=$1;
if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
elif [ $YYYYMMDD = "YESTERDAY" ] ;
then
    YYYYMMDD=$(cat /tmp/YESTERDAY_DATE)
fi

$FETCH_SCRIPT $YYYYMMDD

cd /apps/data/PluralTrades/PluralFiles;
GENERAL_TRD_FILE="/apps/data/PluralTrades/PluralFiles/InvoiceDetailed_$YYYYMMDD.csv";

GENERAL_PNL_FILE_DIR="/apps/data/PluralTrades/Pnl/"${YYYYMMDD:0:4}"/"${YYYYMMDD:4:2}"/"${YYYYMMDD:6:2};
ORS_PNL_DIR="$HOME/recon";

mkdir -p $GENERAL_PNL_FILE_DIR;
mkdir -p $ORS_PNL_DIR;

GENERAL_PNL_FILE=$GENERAL_PNL_FILE_DIR"/general_pnl";
GENERAL_RECON_FILE=$GENERAL_PNL_FILE_DIR"/general_recon";
GENERAL_TRADES_FILE=$GENERAL_PNL_FILE_DIR"/general_trades";

ORS_PNL_FILE=$ORS_PNL_DIR"/plural_ors_pnl";
RESULT_FILE=$ORS_PNL_DIR"/plural_result";
ERROR_FILE=$ORS_PNL_DIR"/plural_err_file";
PNL_RECON_RESULT="/home/dvcinfra/recon/pnl_recon_result"

>$ERROR_FILE
> $RESULT_FILE

if [ ! -f $GENERAL_TRD_FILE ] ; then
    echo "<tr bgcolor='#FF0000'><td colspan=6>File not fetched:  $GENERAL_TRD_FILE</td></tr>" >> $PNL_RECON_RESULT
    echo "<tr bgcolor='#FF0000'><td colspan=6>File not fetched:  $GENERAL_TRD_FILE</td></tr>" >> $RESULT_FILE
    exit 0;
fi

# RTS MICEX # 
$PAST_PNL_SCRIPT $YYYYMMDD > $ORS_PNL_FILE
perl $GENERAL_FORMAT_SCRIPT $GENERAL_TRD_FILE $YYYYMMDD > $GENERAL_TRADES_FILE 2>>$ERROR_FILE
perl $PNL_SCRIPT "R" $GENERAL_TRADES_FILE $YYYYMMDD > $GENERAL_PNL_FILE 2>>$ERROR_FILE

python $COMPARE_SCRIPT $GENERAL_PNL_FILE $ORS_PNL_FILE "Plural" $YYYYMMDD>> $RESULT_FILE

cat $ERROR_FILE | grep RECON >> $RESULT_FILE
if [ -s $RESULT_FILE ]  
then  
    echo >> $RESULT_FILE
    cat $RESULT_FILE >> $PNL_RECON_RESULT
    echo "<tr bgcolor='#FFFFFF'><td colspan=6>&nbsp;</td></tr>" >> $PNL_RECON_RESULT
fi
