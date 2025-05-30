#!/bin/bash

USAGE="$0 YYYYMMDD \n\t >> Generate pnl csvs for date YYYYMMDD.";

if [ $# -ne 1 ] ; 
then 
    echo -e $USAGE;
    exit;
fi


PNL_PER_PROD_LINK_SCRIPT=$HOME/LiveExec/scripts/all_bmf_to_pnl_per_prod_csv.pl;
PNL_PER_EXCH_LINK_SCRIPT=$HOME/LiveExec/scripts/all_bmf_to_pnl_per_exch_csv.pl;

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


YYYYMMDD=$1;
if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
elif [ $YYYYMMDD = "YESTERDAY" ] ;
then
    YYYYMMDD=$(cat /tmp/YESTERDAY_DATE)
fi

# Get the needed files from the NewEdge ftp.
mkdir -p /apps/data/MFGlobalTrades/MFGFiles;
cd /apps/data/MFGlobalTrades/MFGFiles;

# Link files.
cd /apps/data/MFGlobalTrades/MFGFiles;

HOST='ftp.linkinvestimentos.com.br';
USER='dvcapital';
PASSWD='6#A23nds2$';

INVOICE_FILE="Link_InvoiceDetailed_35786_"$YYYYMMDD".csv";
ALPES_INVOICE_FILE="InvoiceDetailled_4505_"$YYYYMMDD".csv";

MARGIN_FILE="Link_Margin_35786_"$YYYYMMDD".csv";
POS_FILE="Link_Position_35786_"$YYYYMMDD".csv";
TRADE_FILE="Link_tradeFeed_35786_"$YYYYMMDD".csv";
COLLATERAL_FILE="Link_Collateral_35786_"$YYYYMMDD".csv";
MOVFUT_FILE="MOVFUT_35786_"${YYYYMMDD:4:2}"_"${YYYYMMDD:6:2}"_"${YYYYMMDD:0:4}".txt";

ftp -n $HOST <<SCRIPT
user $USER $PASSWD
binary
get $INVOICE_FILE
get $MARGIN_FILE
get $POS_FILE
get $TRADE_FILE
get $COLLATERAL_FILE
get $MOVFUT_FILE
quit
SCRIPT

chmod 640 /apps/data/MFGlobalTrades/MFGFiles/*$YYYYMMDD*.csv ;
chmod 640 /apps/data/MFGlobalTrades/MFGFiles/*.txt ;

MFG_TRD_FILE="/apps/data/MFGlobalTrades/MFGFiles/"$IST_FILE;
CSV_PROD_FILE_DIR="/apps/data/MFGlobalTrades/ProductPnl/"${YYYYMMDD:0:4}"/"${YYYYMMDD:4:2}"/"${YYYYMMDD:6:2};
CSV_EXCH_FILE_DIR="/apps/data/MFGlobalTrades/ExchangePnl/"${YYYYMMDD:0:4}"/"${YYYYMMDD:4:2}"/"${YYYYMMDD:6:2};

if [ ! -f $ALPES_INVOICE_FILE ] ; then

    echo -e "MFG trade file :" $ALPES_INVOICE_FILE "does not exist.\nExiting.";

    echo "ALPES INVOICE FILE NOT AVAILABLE"$YYYYMMDD | /bin/mail -s "ALPES INVOICE FILE Missing" -r "generatepnls@ny11" "nseall@tworoads.co.in";
    exit;
fi

if [ ! -s $ALPES_INVOICE_FILE ] 
then

    echo "ALPES INVOICE FILE SIZE ZERO"$YYYYMMDD | /bin/mail -s "ALPES INVOICE FILE Missing" -r "generatepnls@ny11" "nseall@tworoads.co.in";
    exit ;

fi 


mkdir -p $CSV_PROD_FILE_DIR;
mkdir -p $CSV_EXCH_FILE_DIR;

LINK_INVOICE_FILE="/apps/data/MFGlobalTrades/MFGFiles/"$INVOICE_FILE;

echo -e "Reading" $LINK_INVOICE_FILE".\nWriting per prod pnls to "$CSV_PROD_FILE;
`perl -w $PNL_PER_PROD_LINK_SCRIPT $ALPES_INVOICE_FILE $LINK_INVOICE_FILE $YYYYMMDD >> $CSV_PROD_FILE`;

echo -e "Reading" $LINK_INVOICE_FILE".\nWriting per exchange pnls to "$CSV_EXCH_FILE;
`perl -w $PNL_PER_EXCH_LINK_SCRIPT $ALPES_INVOICE_FILE $LINK_INVOICE_FILE $YYYYMMDD >> $CSV_EXCH_FILE`;

rsync -avz /apps/data/MFGlobalTrades/ProductPnl 10.23.74.40:/apps/data/MFGlobalTrades 
rsync -avz /apps/data/MFGlobalTrades/ExchangePnl 10.23.74.40:/apps/data/MFGlobalTrades 

