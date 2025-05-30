#!/bin/bash
LOCKFILE=$HOME/locks/seebmfeqdc.lock
if [ ! -e $LOCKFILE ] ; then
touch $LOCKFILE;

FILE="$HOME/trades/ALL_BMFEQDC_TRADES";
FILE2="$HOME/trades/ALL_BMFEQ_TRADES";
OUT_FILE="$HOME/eq_ors_file.txt";
OUT_FILE2="$HOME/eq_ors_dc_file.txt"

PNL_SCRIPT="$HOME/infracore_install/scripts/see_ors_pnl.pl";

if [ ! -d $HOME/trades ] ;
then
    mkdir $HOME/trades;
fi

while [ true ]
do
    > $FILE;
    > $FILE2;
    > $OUT_FILE2;
    > $OUT_FILE;

    YYYYMMDD=$(date "+%Y%m%d");
    
    if [ $# -eq 1 ] ;
    then
	YYYYMMDD=$1;
    fi
    > $HOME/trades/bmfeqdc11_trades.$YYYYMMDD
    scp -q dvcinfra@10.220.65.34:/spare/local/ORSlogs/BMFEQ/ZBRP/DBRP5006/trades.$YYYYMMDD $HOME/trades/bmfeqdc11_trades.$YYYYMMDD
    cat $HOME/trades/bmfeqdc11_trades.$YYYYMMDD | sed 's/\x7f//g' >> $FILE; 

    > $HOME/trades/bmfeqdc11_trades.$YYYYMMDD
    scp -q dvcinfra@10.220.65.34:/spare/local/ORSlogs/BMFEQ/ZBRP/ZBRP5000/trades.$YYYYMMDD $HOME/trades/bmfeqdc11_trades.$YYYYMMDD
    cat $HOME/trades/bmfeqdc11_trades.$YYYYMMDD | sed 's/\x7f//g' >> $FILE; 

    > $HOME/trades/bmfeqdc11_trades.$YYYYMMDD
    scp -q dvcinfra@10.220.65.34:/spare/local/ORSlogs/BMFEQ/ZBRP/ZBRP5001/trades.$YYYYMMDD $HOME/trades/bmfeqdc11_trades.$YYYYMMDD
    cat $HOME/trades/bmfeqdc11_trades.$YYYYMMDD | sed 's/\x7f//g' >> $FILE; 

#    > $HOME/trades/bmfdc_trades.$YYYYMMDD;

    clear;
    echo " DROPCOPY: " >> $OUT_FILE2
    perl -w $PNL_SCRIPT 'C' $FILE $YYYYMMDD 2 BMFEQ 0 100 >> $OUT_FILE2;

    rsync --timeout=30 -avz  --quiet dvcinfra@10.220.65.34:/spare/local/ORSlogs/BMFEQ/BMFEQ1/trades.$YYYYMMDD   $HOME/trades/bmfeq11.trades.$YYYYMMDD
    cat $HOME/trades/bmfeq11.trades.$YYYYMMDD >> $FILE2;
    echo "ORS: " >> $OUT_FILE;
    perl -w $PNL_SCRIPT 'C' $FILE2 $YYYYMMDD 2 BMFEQ 0 100 >> $OUT_FILE;
    paste $OUT_FILE $OUT_FILE2 | awk -F'\t' '{printf(" %100s \t |\t %100s \n",$1,$2)}' ;
    sleep 20;
done

rm -f $LOCKFILE;
else
echo "$LOCKFILE present. Please delete";
fi
