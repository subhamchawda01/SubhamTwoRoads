#!/bin/bash

USAGE="$0 \n\t >> Generate pnl csvs for date TODAY.";

PNL_PER_PROD_SCRIPT=$HOME/infracore/scripts/newedge_pnl_per_prod_csv.pl;
PNL_PNL_PER_EXCH_SCRIPT=$HOME/infracore/scripts/newedge_pnl_per_exch_csv.pl;

YYYYMMDD=$(date "+%Y%m%d")

HKDATE=`$HOME/infracore_install/bin/calc_prev_day $YYYYMMDD` ;

# Get the needed files from the NewEdge ftp.
mkdir -p /apps/data/MFGlobalTrades/MFGFiles;
cd /apps/data/MFGlobalTrades/MFGFiles;

HOST='Ftp.newedgegroup.com';
USER='DV_CAPITAL@USA';
PASSWD='GNVVfbY8';

TRN_FILE="GMITRN_"$YYYYMMDD".csv";

ftp -n $HOST <<SCRIPT
user $USER $PASSWD
binary
get $TRN_FILE
quit
SCRIPT

cd /apps/data/MFGlobalTrades/MFGFiles;

chmod 644 /apps/data/MFGlobalTrades/MFGFiles/*$YYYYMMDD*.csv ;
chmod 644 /apps/data/MFGlobalTrades/MFGFiles/*.txt ;

MFG_TRD_FILE="/apps/data/MFGlobalTrades/MFGFiles/"$TRN_FILE;

#CSV_EXCH_FILE_DIR="/apps/data/MFGlobalTrades/ExchangePnl/"${YYYYMMDD:0:4}"/"${YYYYMMDD:4:2}"/"${YYYYMMDD:6:2};
#CSV_HK_EXCH_FILE_DIR="/apps/data/MFGlobalTrades/ExchangePnl/"${HKDATE:0:4}"/"${HKDATE:4:2}"/"${HKDATE:6:2};

if [ ! -f $MFG_TRD_FILE ] ; then

    echo -e "MFG trade file :" $MFG_TRD_FILE "does not exist.\nExiting.";
    echo "GMIST4 FILE NOT AVAILABLE"$YYYYMMDD | /bin/mail -s "GMIST4 FILE Missing" -r "generatepnls@ny11" "kp@circulumvite.com";

fi

if [ ! -s $MFG_TRD_FILE ] 
then

    echo "GMIST4 FILE SIZE ZERO"$YYYYMMDD | /bin/mail -s "GMIST4 FILE Missing" -r "generatepnls@ny11" "kp@circulumvite.com";

fi 

#`perl $PNL_PER_PROD_SCRIPT $MFG_TRD_FILE $YYYYMMDD ONLINE`;

#CSV_EXCH_FILE=$CSV_EXCH_FILE_DIR"/pnl.csv";
#CSV_HK_EXCH_FILE=$CSV_HK_EXCH_FILE_DIR"/pnl.csv";

#echo -e "Reading" $MFG_TRD_FILE".\nWriting per exchange pnls to "$CSV_EXCH_FILE;
#`perl -w $PNL_PER_EXCH_SCRIPT $MFG_TRD_FILE $YYYYMMDD > $CSV_EXCH_FILE`;
#`perl -w $HK_PNL_PER_EXCH_SCRIPT $MFG_TRD_FILE $HKDATE >> $CSV_HK_EXCH_FILE`;

#rsync -avz /apps/data/MFGlobalTrades/ProductPnl 10.23.74.40:/apps/data/MFGlobalTrades 
#rsync -avz /apps/data/MFGlobalTrades/ExchangePnl 10.23.74.40:/apps/data/MFGlobalTrades 

