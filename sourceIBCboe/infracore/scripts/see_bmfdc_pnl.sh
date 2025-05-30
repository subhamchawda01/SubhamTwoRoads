#!/bin/bash
LOCKFILE=$HOME/locks/seebmfdc.lock
if [ ! -e $LOCKFILE ] ; then
touch $LOCKFILE;

FILE="$HOME/trades/ALL_BMFDC_TRADES";
PNL_SCRIPT="$HOME/infracore_install/scripts/see_ors_pnl.pl";

if [ ! -d $HOME/trades ] ;
then
    mkdir $HOME/trades;
fi

while [ true ]
do
    > $FILE;

    YYYYMMDD=$(date "+%Y%m%d");
    
    if [ $# -eq 1 ] ;
    then
	YYYYMMDD=$1;
    fi

#    scp -q dvcinfra@10.220.65.34:/spare/local/ORSlogs/BMFEP/ZLIN12/trades.$YYYYMMDD $HOME/trades/bmfdc_trades.$YYYYMMDD
#    cat $HOME/trades/bmfdc_trades.$YYYYMMDD >> $FILE; 

#    > $HOME/trades/bmfdc_trades.$YYYYMMDD;

    GUI_TRADES_FILE=$HOME/trades/bmf_rodrigo_trades.$YYYYMMDD
    if [ -e $GUI_TRADES_FILE ] ; then 
	cat $GUI_TRADES_FILE >> $FILE ;
    fi

# rodrigo prices first
    scp -q dvcinfra@10.220.65.34:/spare/local/ORSlogs/BMFEP/RETAIL/trades.$YYYYMMDD $HOME/trades/bmfdc12_trades.$YYYYMMDD
    cat $HOME/trades/bmfdc12_trades.$YYYYMMDD >> $FILE; 

#    scp -q dvcinfra@10.220.65.34:/spare/local/ORSlogs/BMFEP/DALP0012/trades.$YYYYMMDD $HOME/trades/bmfdc11_trades.$YYYYMMDD
#   cat $HOME/trades/bmfdc11_trades.$YYYYMMDD >> $FILE; 
    
    scp -q dvcinfra@10.220.65.34:/spare/local/ORSlogs/BMFEP/DBRP0004/trades.$YYYYMMDD $HOME/trades/bmfdc11_trades.$YYYYMMDD
    cat $HOME/trades/bmfdc11_trades.$YYYYMMDD >> $FILE; 

#    > $HOME/trades/bmfdc_trades.$YYYYMMDD;

    clear;
    perl -w $PNL_SCRIPT 'C' $FILE $YYYYMMDD 2 BMF

    sleep 30;
done

rm -f $LOCKFILE;
else
echo "$LOCKFILE present. Please delete";
fi
