#!/bin/bash

USAGE="$0 YYYYMMDD \n\t >> Generate pnl csvs for date YYYYMMDD.";

if [ $# -ne 1 ] ; 
then 
    echo -e $USAGE;
    exit;
fi

GENERAL_FORMAT_SCRIPT=$HOME/infracore/recon/get_general_newedge_eod_trades_in_ors_format.pl;
HK_FORMAT_SCRIPT=$HOME/infracore/recon/get_hk_newedge_eod_trades_in_ors_format.pl;
OSE_FORMAT_SCRIPT=$HOME/infracore/recon/get_ose_newedge_eod_trades_in_ors_format.pl;
SGX_FORMAT_SCRIPT=$HOME/infracore/recon/get_sgx_newedge_eod_trades_in_ors_format.pl;
PNL_SCRIPT=$HOME/infracore_install/scripts/see_ors_pnl.pl;
PAST_PNL_SCRIPT=$HOME/LiveExec/scripts/get_EOD_pnl.sh;
COMPARE_SCRIPT=$HOME/infracore/recon/compare_broker_ors_eod_pnl.py;
FETCH_SCRIPT=$HOME/infracore/recon/fetch_newedge_files.sh

YYYYMMDD=$1;
if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
elif [ $YYYYMMDD = "YESTERDAY" ] ;
then
    YYYYMMDD=$(cat /tmp/YESTERDAY_DATE)
fi

MNY_FILE="GMIMNY_"$YYYYMMDD".csv";
POS_FILE="GMIPOS_"$YYYYMMDD".csv";
IST_FILE="GMIST4_"$YYYYMMDD".csv";
TRN_FILE="GMITRN_"$YYYYMMDD".csv";

HKDATE=`$HOME/infracore_install/bin/calc_prev_week_day $YYYYMMDD` ;
TOKDATE=`$HOME/infracore_install/bin/calc_prev_week_day $YYYYMMDD` ;
MOSDATE=$YYYYMMDD; #`$HOME/infracore_install/bin/calc_prev_week_day $YYYYMMDD` ;

$FETCH_SCRIPT $YYYYMMDD

# Link files.
cd /apps/data/MFGlobalTrades/MFGFiles;

chmod 644 /apps/data/MFGlobalTrades/MFGFiles/*$YYYYMMDD*.csv ;


NE_GENERAL_TRD_FILE="/apps/data/MFGlobalTrades/MFGFiles/"$TRN_FILE;

GENERAL_PNL_FILE_DIR="/apps/data/MFGlobalTrades/Pnl/"${YYYYMMDD:0:4}"/"${YYYYMMDD:4:2}"/"${YYYYMMDD:6:2};
HK_PNL_FILE_DIR="/apps/data/MFGlobalTrades/Pnl/"${HKDATE:0:4}"/"${HKDATE:4:2}"/"${HKDATE:6:2};
OSE_PNL_FILE_DIR="/apps/data/MFGlobalTrades/Pnl/"${TOKDATE:0:4}"/"${TOKDATE:4:2}"/"${TOKDATE:6:2};
SGX_PNL_FILE_DIR="/apps/data/MFGlobalTrades/Pnl/"${TOKDATE:0:4}"/"${TOKDATE:4:2}"/"${TOKDATE:6:2};
ORS_PNL_DIR="$HOME/recon";


mkdir -p $GENERAL_PNL_FILE_DIR;
mkdir -p $HK_PNL_FILE_DIR;
mkdir -p $OSE_PNL_FILE_DIR;
mkdir -p $SGX_PNL_FILE_DIR;
mkdir -p $ORS_PNL_DIR;

GENERAL_PNL_FILE=$GENERAL_PNL_FILE_DIR"/general_pnl";
HK_PNL_FILE=$HK_PNL_FILE_DIR"/hk_pnl";
OSE_PNL_FILE=$OSE_PNL_FILE_DIR"/ose_pnl";
SGX_PNL_FILE=$SGX_PNL_FILE_DIR"/sgx_pnl";

GENERAL_RECON_FILE=$GENERAL_PNL_FILE_DIR"/general_recon";
HK_RECON_FILE=$HK_PNL_FILE_DIR"/hk_recon";
OSE_RECON_FILE=$OSE_PNL_FILE_DIR"/ose_recon";
SGX_RECON_FILE=$SGX_PNL_FILE_DIR"/sgx_recon";

GENERAL_TRADES_FILE=$GENERAL_PNL_FILE_DIR"/general_trades";
HK_TRADES_FILE=$HK_PNL_FILE_DIR"/hk_trades";
OSE_TRADES_FILE=$OSE_PNL_FILE_DIR"/ose_trades";
SGX_TRADES_FILE=$SGX_PNL_FILE_DIR"/sgx_trades";

ORS_PNL_FILE=$ORS_PNL_DIR"/newedge_ors_pnl";
RESULT_FILE=$ORS_PNL_DIR"/newedge_result";
ERROR_FILE=$ORS_PNL_DIR"/newedge_err_file";

PNL_RECON_RESULT="/home/dvcinfra/recon/pnl_recon_result"

>$ERROR_FILE
>$RESULT_FILE

if [ ! -f $NE_GENERAL_TRD_FILE ] ; then
    echo "<tr> bgcolor='#FF0000'<td colspan=6>File not fetched:  $NE_GENERAL_TRD_FILE</td></tr>" >> $PNL_RECON_RESULT
    echo "<tr> bgcolor='#FF0000'<td colspan=6>File not fetched:  $NE_GENERAL_TRD_FILE</td></tr>" >> $RESULT_FILE
fi

$PAST_PNL_SCRIPT $YYYYMMDD > $ORS_PNL_FILE

# TMX BMF EUREX LIFFE CME#
perl $GENERAL_FORMAT_SCRIPT $NE_GENERAL_TRD_FILE $YYYYMMDD > $GENERAL_TRADES_FILE 2>>$ERROR_FILE
sed -i 's/~/ /g' $GENERAL_TRADES_FILE
perl $PNL_SCRIPT "R" $GENERAL_TRADES_FILE $YYYYMMDD > $GENERAL_PNL_FILE 2>>$ERROR_FILE
python $COMPARE_SCRIPT $GENERAL_PNL_FILE $ORS_PNL_FILE "Newedge" $YYYYMMDD>> $RESULT_FILE
echo "<tr bgcolor='#FFFFFF'><td colspan=6>&nbsp;</td></tr>" >> $RESULT_FILE

# OSE # 
perl $OSE_FORMAT_SCRIPT $YYYYMMDD > $OSE_TRADES_FILE 2>>$ERROR_FILE
sed -i 's/~/ /g' $OSE_TRADES_FILE
perl $PNL_SCRIPT "R" $OSE_TRADES_FILE $YYYYMMDD > $OSE_PNL_FILE 2>>$ERROR_FILE
python $COMPARE_SCRIPT $OSE_PNL_FILE $ORS_PNL_FILE "Newedge" $YYYYMMDD>> $RESULT_FILE
echo "<tr bgcolor='#FFFFFF'><td colspan=6>&nbsp;</td></tr>">> $RESULT_FILE

# SGX # 
perl $SGX_FORMAT_SCRIPT $YYYYMMDD > $SGX_TRADES_FILE 2>>$ERROR_FILE
sed -i 's/~/ /g' $SGX_TRADES_FILE
perl $PNL_SCRIPT "R" $SGX_TRADES_FILE $YYYYMMDD > $SGX_PNL_FILE 2>>$ERROR_FILE
python $COMPARE_SCRIPT $SGX_PNL_FILE $ORS_PNL_FILE "Newedge" $YYYYMMDD>> $RESULT_FILE
echo "<tr bgcolor='#FFFFFF'><td colspan=6>&nbsp;</td></tr>">> $RESULT_FILE

# HK # 
perl $HK_FORMAT_SCRIPT $YYYYMMDD > $HK_TRADES_FILE 2>>$ERROR_FILE
sed -i 's/~/ /g' $HK_TRADES_FILE
perl $PNL_SCRIPT "R" $HK_TRADES_FILE $YYYYMMDD > $HK_PNL_FILE 2>>$ERROR_FILE
python $COMPARE_SCRIPT $HK_PNL_FILE $ORS_PNL_FILE "Newedge" $YYYYMMDD>> $RESULT_FILE

cat $ERROR_FILE | grep RECON >> $RESULT_FILE
if [ -s $RESULT_FILE ]  
then  
    echo >> $RESULT_FILE
    cat $RESULT_FILE >> $PNL_RECON_RESULT
    echo "<tr bgcolor='#FFFFFF'><td colspan=6>&nbsp;</td></tr>" >> $PNL_RECON_RESULT
fi
