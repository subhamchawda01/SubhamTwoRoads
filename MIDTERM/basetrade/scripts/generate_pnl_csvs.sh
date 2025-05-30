#!/bin/bash

USAGE="$0 YYYYMMDD \n\t >> Generate pnl csvs for date YYYYMMDD.";

if [ $# -ne 1 ] ; 
then 
    echo -e $USAGE;
    exit;
fi

PNL_PER_PROD_SCRIPT=$HOME/LiveExec/scripts/mfg_to_pnl_per_prod_csv.pl;
PNL_PER_EXCH_SCRIPT=$HOME/LiveExec/scripts/mfg_to_pnl_per_exch_csv.pl;

YYYYMMDD=$1;
if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
elif [ $YYYYMMDD = "YESTERDAY" ] ;
then
    YYYYMMDD=$(cat /tmp/YESTERDAY_DATE)
fi

# Get the needed files from the NewEdge ftp.
mkdir -p /NAS1/data/MFGlobalTrades/MFGFiles;
cd /NAS1/data/MFGlobalTrades/MFGFiles;

HOST='Ftp.newedgegroup.com';
USER='DV_CAPITAL@USA';
PASSWD='GNVVfbY8';

MNY_FILE="GMIMNY_"$YYYYMMDD".csv";
POS_FILE="GMIPOS_"$YYYYMMDD".csv";
IST_FILE="GMIST4_"$YYYYMMDD".csv";
TRN_FILE="GMITRN_"$YYYYMMDD".csv";

ftp -n $HOST <<SCRIPT
user $USER $PASSWD
binary
get $MNY_FILE
get $POS_FILE
get $IST_FILE
get $TRN_FILE
quit
SCRIPT

chmod 644 /NAS1/data/MFGlobalTrades/MFGFiles/*$YYYYMMDD*.csv ;

MFG_TRD_FILE="/NAS1/data/MFGlobalTrades/MFGFiles/"$IST_FILE;
CSV_PROD_FILE_DIR="/NAS1/data/MFGlobalTrades/ProductPnl/"${YYYYMMDD:0:4}"/"${YYYYMMDD:4:2}"/"${YYYYMMDD:6:2};
CSV_EXCH_FILE_DIR="/NAS1/data/MFGlobalTrades/ExchangePnl/"${YYYYMMDD:0:4}"/"${YYYYMMDD:4:2}"/"${YYYYMMDD:6:2};

if [ ! -f $MFG_TRD_FILE ] ; then
    echo -e "MFG trade file :" $MFG_TRD_FILE "does not exist.\nExiting.";
    exit;
fi

mkdir -p $CSV_PROD_FILE_DIR;
mkdir -p $CSV_EXCH_FILE_DIR;

CSV_PROD_FILE=$CSV_PROD_FILE_DIR"/pnl.csv";

echo -e "Reading" $MFG_TRD_FILE".\nWriting per product pnls to "$CSV_PROD_FILE;
`perl -w $PNL_PER_PROD_SCRIPT $MFG_TRD_FILE $YYYYMMDD > $CSV_PROD_FILE`;

CSV_EXCH_FILE=$CSV_EXCH_FILE_DIR"/pnl.csv";

echo -e "Reading" $MFG_TRD_FILE".\nWriting per exchange pnls to "$CSV_EXCH_FILE;
`perl -w $PNL_PER_EXCH_SCRIPT $MFG_TRD_FILE $YYYYMMDD > $CSV_EXCH_FILE`;

