#!/bin/bash

USAGE="$0 YYYYMMDD \n\t >> Generate pnl csvs for date YYYYMMDD.";

if [ $# -ne 1 ] ; 
then 
    echo -e $USAGE;
    exit;
fi

PNL_PER_PROD_SCRIPT=$HOME/LiveExec/scripts/mfg_to_pnl_per_prod_csv.pl;
PNL_PER_EXCH_SCRIPT=$HOME/LiveExec/scripts/mfg_to_pnl_per_exch_csv.pl;

PNL_PER_PROD_LINK_SCRIPT=$HOME/LiveExec/scripts/all_bmf_to_pnl_per_prod_csv.pl;
PNL_PER_EXCH_LINK_SCRIPT=$HOME/LiveExec/scripts/all_bmf_to_pnl_per_exch_csv.pl;

HK_PNL_PER_PROD_SCRIPT=$HOME/LiveExec/scripts/newedge_hk_pnl_trn.pl;
HK_PNL_PER_EXCH_SCRIPT=$HOME/LiveExec/scripts/newedge_hk_exch_pnl_trn.pl;

OSE_PNL_PER_PROD_SCRIPT=$HOME/LiveExec/scripts/newedge_ose_pnl_trn.pl;
OSE_PNL_PER_EXCH_SCRIPT=$HOME/LiveExec/scripts/newedge_ose_exch_pnl_trn.pl;

BCS_PNL_SCRIPT=$HOME/LiveExec/scripts/bcs_trades_to_pnl_csv.pl;


YYYYMMDD=$1;
if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
elif [ $YYYYMMDD = "YESTERDAY" ] ;
then
    YYYYMMDD=$(cat /tmp/YESTERDAY_DATE)
fi

HKDATE=`$HOME/infracore_install/bin/calc_prev_week_day $YYYYMMDD` ;
TOKDATE=`$HOME/infracore_install/bin/calc_prev_week_day $YYYYMMDD` ;
MOSDATE=$YYYYMMDD; #`$HOME/infracore_install/bin/calc_prev_week_day $YYYYMMDD` ;
#TOKDATE='20130517'

# Get the needed files from the NewEdge ftp.
mkdir -p /apps/data/MFGlobalTrades/MFGFiles;
cd /apps/data/MFGlobalTrades/MFGFiles;

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

# Link files.
cd /apps/data/MFGlobalTrades/MFGFiles;

ALPES_INVOICE_FILE1="InvoiceDetailed_4505_"$YYYYMMDD".csv";
ALPES_INVOICE_FILE2="InvoiceDetailed_4972_"$YYYYMMDD".csv";
ALPES_INVOICE_FILE3="InvoiceDetailed_4994_"$YYYYMMDD".csv";

ALPES_INVOICE_FILE="/NAS1/data/MFGlobalTrades/MFGFiles/InvoiceDetailed_"$YYYYMMDD".csv";

if [ ! -f $ALPES_INVOICE_FILE1 ] ; then
    ALPES_INVOICE_FILE1="InvoiceDetailled_4505_"$YYYYMMDD".csv";
fi

if [ ! -f $ALPES_INVOICE_FILE2 ] ; then
    ALPES_INVOICE_FILE2="InvoiceDetailled_4972_"$YYYYMMDD".csv";
fi

if [ ! -f $ALPES_INVOICE_FILE3 ] ; then
    ALPES_INVOICE_FILE3="InvoiceDetailled_4994_"$YYYYMMDD".csv";
fi

#`cat $ALPES_INVOICE_FILE1 $ALPES_INVOICE_FILE2 $ALPES_INVOICE_FILE3 | grep "ISIN_CODE" | sort | uniq > $ALPES_INVOICE_FILE 2>/dev/null`;
#`cat $ALPES_INVOICE_FILE1 $ALPES_INVOICE_FILE2 $ALPES_INVOICE_FILE3 | grep -v "ISIN_CODE" >> $ALPES_INVOICE_FILE 2>/dev/null`;

chmod 644 /apps/data/MFGlobalTrades/MFGFiles/*$YYYYMMDD*.csv ;
chmod 644 /apps/data/MFGlobalTrades/MFGFiles/*.txt ;

MFG_TRD_FILE="/apps/data/MFGlobalTrades/MFGFiles/"$IST_FILE;

CSV_PROD_FILE_DIR="/apps/data/MFGlobalTrades/ProductPnl/"${YYYYMMDD:0:4}"/"${YYYYMMDD:4:2}"/"${YYYYMMDD:6:2};
CSV_EXCH_FILE_DIR="/apps/data/MFGlobalTrades/ExchangePnl/"${YYYYMMDD:0:4}"/"${YYYYMMDD:4:2}"/"${YYYYMMDD:6:2};

CSV_HK_PROD_FILE_DIR="/apps/data/MFGlobalTrades/ProductPnl/"${HKDATE:0:4}"/"${HKDATE:4:2}"/"${HKDATE:6:2};
CSV_HK_EXCH_FILE_DIR="/apps/data/MFGlobalTrades/ExchangePnl/"${HKDATE:0:4}"/"${HKDATE:4:2}"/"${HKDATE:6:2};

CSV_OSE_PROD_FILE_DIR="/apps/data/MFGlobalTrades/ProductPnl/"${TOKDATE:0:4}"/"${TOKDATE:4:2}"/"${TOKDATE:6:2};
CSV_OSE_EXCH_FILE_DIR="/apps/data/MFGlobalTrades/ExchangePnl/"${TOKDATE:0:4}"/"${TOKDATE:4:2}"/"${TOKDATE:6:2};

CSV_MOS_PROD_FILE_DIR="/apps/data/MFGlobalTrades/ProductPnl/"${MOSDATE:0:4}"/"${MOSDATE:4:2}"/"${MOSDATE:6:2};
CSV_MOS_EXCH_FILE_DIR="/apps/data/MFGlobalTrades/ExchangePnl/"${MOSDATE:0:4}"/"${MOSDATE:4:2}"/"${MOSDATE:6:2};

if [ ! -f $MFG_TRD_FILE ] ; then
    echo -e "MFG trade file :" $MFG_TRD_FILE "does not exist.\nExiting.";
    echo "GMIST4 FILE NOT AVAILABLE"$YYYYMMDD | /bin/mail -s "GMIST4 FILE Missing" -r "generatepnls@ny11" "nseall@tworoads.co.in";
fi

if [ ! -s $MFG_TRD_FILE ] ; then
    echo "GMIST4 FILE SIZE ZERO"$YYYYMMDD | /bin/mail -s "GMIST4 FILE Missing" -r "generatepnls@ny11" "nseall@tworoads.co.in";
fi

if [ ! -f $ALPES_INVOICE_FILE ] ; then
    ALPES_INVOICE_FILE="InvoiceDetailled_ALL_"$YYYYMMDD".csv";    
    if [ ! -f $ALPES_INVOICE_FILE ] ; then
	echo -e "MFG trade files :" $ALPES_INVOICE_FILE  "does not exist.\nExiting.";
#	echo "ALPES INVOICE FILE NOT AVAILABLE"$YYYYMMDD | /bin/mail -s "ALPES INVOICE FILE Missing" -r "generatepnls@ny11" "nseall@tworoads.co.in";
    fi
fi

mkdir -p $CSV_PROD_FILE_DIR;
mkdir -p $CSV_EXCH_FILE_DIR;

CSV_PROD_FILE=$CSV_PROD_FILE_DIR"/pnl.csv";
CSV_EXCH_FILE=$CSV_EXCH_FILE_DIR"/pnl.csv";

CSV_HK_EXCH_FILE=$CSV_HK_EXCH_FILE_DIR"/pnl.csv";
CSV_HK_PROD_FILE=$CSV_HK_PROD_FILE_DIR"/pnl.csv";

CSV_OSE_EXCH_FILE=$CSV_OSE_EXCH_FILE_DIR"/pnl.csv";
CSV_OSE_PROD_FILE=$CSV_OSE_PROD_FILE_DIR"/pnl.csv";

CSV_MOS_EXCH_FILE=$CSV_MOS_EXCH_FILE_DIR"/pnl.csv";
CSV_MOS_PROD_FILE=$CSV_MOS_PROD_FILE_DIR"/pnl.csv";


# TMX CME BMF EUREX LIFFE # 
echo -e "Reading" $MFG_TRD_FILE".\nWriting per product pnls to "$CSV_PROD_FILE;
`perl -w $PNL_PER_PROD_SCRIPT $MFG_TRD_FILE $YYYYMMDD > $CSV_PROD_FILE`;
echo -e "Reading" $MFG_TRD_FILE".\nWriting per exchange pnls to "$CSV_EXCH_FILE;
`perl -w $PNL_PER_EXCH_SCRIPT $MFG_TRD_FILE $YYYYMMDD > $CSV_EXCH_FILE`;

LINK_INVOICE_FILE="/apps/data/MFGlobalTrades/MFGFiles/"$INVOICE_FILE;
echo -e "Reading" $ALPES_INVOICE_FILE".\nWriting per prod pnls to "$CSV_PROD_FILE;
`perl -w $PNL_PER_PROD_LINK_SCRIPT $ALPES_INVOICE_FILE $LINK_INVOICE_FILE $YYYYMMDD >> $CSV_PROD_FILE`;
echo -e "Reading" $ALPES_INVOICE_FILE".\nWriting per exchange pnls to "$CSV_EXCH_FILE;
`perl -w $PNL_PER_EXCH_LINK_SCRIPT $ALPES_INVOICE_FILE $LINK_INVOICE_FILE $YYYYMMDD >> $CSV_EXCH_FILE`;
# TMX CME BMF EUREX LIFFE  # 

# HKEX #
echo -e "Writing HK pnls to $CSV_HK_PROD_FILE "
`perl -w $HK_PNL_PER_PROD_SCRIPT $HKDATE > /tmp/csvs.tmp`;
for key in `cat /tmp/csvs.tmp | awk -F, '{print $2}'` ; do sed -i "/$key/d" $CSV_HK_PROD_FILE  ; done
cat /tmp/csvs.tmp >> $CSV_HK_PROD_FILE ;
`>/tmp/csvs.tmp`
echo -e "Writing HK pnls to $CSV_HK_EXCH_FILE "
`perl -w $HK_PNL_PER_EXCH_SCRIPT $HKDATE > /tmp/csvs.tmp`;
for key in `cat /tmp/csvs.tmp | awk -F, '{print $2}'` ; do sed -i "/$key/d" $CSV_HK_EXCH_FILE  ; done
cat /tmp/csvs.tmp >> $CSV_HK_EXCH_FILE ;
`>/tmp/csvs.tmp`
# HKEX # 

# OSE #
echo -e "Writing OSE pnls to $CSV_OSE_PROD_FILE "
`perl -w $OSE_PNL_PER_PROD_SCRIPT $TOKDATE > /tmp/csvs.tmp`
for key in `cat /tmp/csvs.tmp | awk -F, '{print $2}'` ; do sed -i "/$key/d" $CSV_OSE_PROD_FILE  ; done
`cat /tmp/csvs.tmp >> $CSV_OSE_PROD_FILE`;
`>/tmp/csvs.tmp`
echo -e "Writing OSE pnls to $CSV_OSE_EXCH_FILE "
`perl -w $OSE_PNL_PER_EXCH_SCRIPT $TOKDATE > /tmp/csvs.tmp`
for key in `cat /tmp/csvs.tmp | awk -F, '{print $2}'` ; do sed -i "/$key/d" $CSV_OSE_EXCH_FILE  ; done
`cat /tmp/csvs.tmp >> $CSV_OSE_EXCH_FILE`;
`>/tmp/csvs.tmp`
# OSE # 


# RTS MICEX # 
#/NAS1/broker_files/BCS_MOS/20131009/10396_trades_20131009_111530.csv
#/NAS1/broker_files/BCS_MOS/20131009/8279396_trades_20131009_111533.csv

BCS_TRADES_FILE1="/NAS1/broker_files/BCS_MOS/$YYYYMMDD/10396_trades_$YYYYMMDD_*.csv";
BCS_TRADES_FILE2="/NAS1/broker_files/BCS_MOS/$YYYYMMDD/8279396_trades_$YYYYMMDD_*.csv";

if [ -f $BCS_TRADES_FILE1 ];
then
    echo -e "Writing MOS pnls to $CSV_MOS_PROD_FILE "
    `perl -w $BCS_PNL_SCRIPT $BCS_TRADES_FILE1 $MOSDATE 1 > /tmp/csvs.tmp`;
    echo -e "perl -w $BCS_PNL_SCRIPT $BCS_TRADES_FILE1 $MOSDATE 1 > /tmp/csvs.tmp";
    for key in `cat /tmp/csvs.tmp | awk -F, '{print $2}'` ; do sed -i "/$key/d" $CSV_MOS_PROD_FILE  ; done
    `cat /tmp/csvs.tmp >> $CSV_MOS_PROD_FILE`;
    `>/tmp/csvs.tmp`
    echo -e "Writing MOS pnls to $CSV_MOS_EXCH_FILE "
    `perl -w $BCS_PNL_SCRIPT $BCS_TRADES_FILE1 $MOSDATE 0 > /tmp/csvs.tmp`;
    for key in `cat /tmp/csvs.tmp | awk -F, '{print $2}'` ; do sed -i "/$key/d" $CSV_MOS_EXCH_FILE  ; done
    `cat /tmp/csvs.tmp >> $CSV_MOS_EXCH_FILE`;
    `>/tmp/csvs.tmp`
fi
if [ -f $BCS_TRADES_FILE2 ];
then
    echo -e "Writing MOS pnls to $CSV_MOS_PROD_FILE "
    `perl -w $BCS_PNL_SCRIPT $BCS_TRADES_FILE2 $MOSDATE 1 > /tmp/csvs.tmp`;
    echo -e "perl -w $BCS_PNL_SCRIPT $BCS_TRADES_FILE2 $MOSDATE 1 > /tmp/csvs.tmp";
    for key in `cat /tmp/csvs.tmp | awk -F, '{print $2}'` ; do sed -i "/$key/d" $CSV_MOS_PROD_FILE  ; done
    `cat /tmp/csvs.tmp >> $CSV_MOS_PROD_FILE`;
    `>/tmp/csvs.tmp`
    echo -e "Writing MOS pnls to $CSV_MOS_EXCH_FILE "
    `perl -w $BCS_PNL_SCRIPT $BCS_TRADES_FILE2 $MOSDATE 0 > /tmp/csvs.tmp`;
    for key in `cat /tmp/csvs.tmp | awk -F, '{print $2}'` ; do sed -i "/$key/d" $CSV_MOS_EXCH_FILE  ; done
    `cat /tmp/csvs.tmp >> $CSV_MOS_EXCH_FILE`;
    `>/tmp/csvs.tmp`
fi
# RTS MICEX #

rsync -avz /apps/data/MFGlobalTrades/ProductPnl 10.23.74.40:/apps/data/MFGlobalTrades 
rsync -avz /apps/data/MFGlobalTrades/ExchangePnl 10.23.74.40:/apps/data/MFGlobalTrades 
rsync -avz /apps/data/MFGlobalTrades/MFGFiles 10.23.74.40:/apps/data/MFGlobalTrades
