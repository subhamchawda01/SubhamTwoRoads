#!/bin/bash

# Remove all temporary files created by see_ors_pnl.pl (for storing symbol->remaining days map) before exitting
trap 'rm -rf  $HOME/.DI_remaining_days.*' EXIT

LOCKFILE=$HOME/locks/seeallpnls.lock;
if [ ! -e $LOCKFILE ] ; then
    touch $LOCKFILE;

    FILE="$HOME/trades/ALL_TRADES";

    trade_date_=$(date "+%Y%m%d");
    CME_EU_VOL_INFO_FILE=$HOME/trades/cme_eu_vol_info_file_$trade_date_ ;

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

	GUI_TRADES_FILE=$HOME/trades/gui_trades.$YYYYMMDD;
	if [ -e $GUI_TRADES_FILE ] ; then 
	    cat $GUI_TRADES_FILE >> $FILE ;
	fi
	
    # scp -q dvcinfra@sdv-chi-srv11:/spare/local/ORSlogs/CME/HC0/trades.$YYYYMMDD $HOME/trades
    # cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

    # scp -q dvcinfra@sdv-chi-srv13:/spare/local/ORSlogs/CME/G52/trades.$YYYYMMDD $HOME/trades
    # cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

    # scp -q dvcinfra@sdv-chi-srv14:/spare/local/ORSlogs/CME/VD4/trades.$YYYYMMDD $HOME/trades
    # cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;


    #CME servers
	rsync --timeout=30 -avz  --quiet dvcinfra@10.23.82.52:/spare/local/ORSlogs/CME/*/trades.$YYYYMMDD   $HOME/trades/cme12.trades.$YYYYMMDD    
	cat $HOME/trades/cme12.trades.$YYYYMMDD >> $FILE;

	rsync --timeout=30 -avz  --quiet dvcinfra@10.23.82.53:/spare/local/ORSlogs/CME/*/trades.$YYYYMMDD   $HOME/trades/cme13.trades.$YYYYMMDD    
	cat $HOME/trades/cme13.trades.$YYYYMMDD >> $FILE;

	rsync --timeout=30 -avz  --quiet dvcinfra@10.23.82.54:/spare/local/ORSlogs/CME/*/trades.$YYYYMMDD   $HOME/trades/cme14.trades.$YYYYMMDD    
	cat $HOME/trades/cme14.trades.$YYYYMMDD >> $FILE;

	rsync --timeout=30 -avz  --quiet dvcinfra@10.23.82.51:/spare/local/ORSlogs/CME/*/trades.$YYYYMMDD   $HOME/trades/cme11.trades.$YYYYMMDD    
	cat $HOME/trades/cme11.trades.$YYYYMMDD >> $FILE;
	
        rsync --timeout=30 -avz  --quiet dvcinfra@10.23.82.55:/spare/local/ORSlogs/CME/*/trades.$YYYYMMDD   $HOME/trades/cme15.trades.$YYYYMMDD    
	cat $HOME/trades/cme15.trades.$YYYYMMDD >> $FILE;
	
#        rsync --timeout=30 -avz  --quiet dvcinfra@10.23.82.56:/spare/local/ORSlogs/CME/*/trades.$YYYYMMDD   $HOME/trades/cme16.trades.$YYYYMMDD    
#	cat $HOME/trades/cme16.trades.$YYYYMMDD >> $FILE;


#	rsync --timeout=30 -avz  --quiet dvcinfra@10.23.82.53:/home/spare/local/ORSlogs/CME/CMECHI13/trades.$YYYYMMDD   $HOME/trades/cme13.trades.$YYYYMMDD
#	cat $HOME/trades/cme13.trades.$YYYYMMDD >> $FILE;

	trade_hours_=$(date "+%H");

	if [ $trade_hours_ -gt 11 ]
	then

            if [ ! -f $CME_EU_VOL_INFO_FILE ]
	    then

		touch $CME_EU_VOL_INFO_FILE
		>$CME_EU_VOL_INFO_FILE

		echo "ZN " `grep "ZN" $HOME/trades/cme11.trades.$YYYYMMDD | wc -l` >> $CME_EU_VOL_INFO_FILE
		echo "UB " `grep "UB" $HOME/trades/cme12.trades.$YYYYMMDD | wc -l` >> $CME_EU_VOL_INFO_FILE
		echo "ZB " `grep "ZB" $HOME/trades/cme12.trades.$YYYYMMDD | wc -l` >> $CME_EU_VOL_INFO_FILE
		echo "ZF " `grep "ZF" $HOME/trades/cme13.trades.$YYYYMMDD | wc -l` >> $CME_EU_VOL_INFO_FILE

	    fi

	fi

    #EUREX server
    # scp -q dvcinfra@sdv-fr2-srv11:/spare/local/ORSlogs/EUREX/NTAPROD2/trades.$YYYYMMDD $HOME/trades
    # cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

    # scp -q dvcinfra@sdv-fr2-srv14:/spare/local/ORSlogs/EUREX/NTAPROD3/trades.$YYYYMMDD $HOME/trades
    # cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

    # scp -q dvcinfra@sdv-fr2-srv13:/spare/local/ORSlogs/EUREX/NTAPROD3/trades.$YYYYMMDD $HOME/trades
    # cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;



    #EUREX server
    #This way we will have a backup to revert to the last known file if rsync --timeout=30 fails on the network
    #scp seems to be costly
    rsync --timeout=30 -avz  --quiet dvcinfra@10.23.200.51:/spare/local/ORSlogs/EUREX/*/trades.$YYYYMMDD   $HOME/trades/fr11.trades.$YYYYMMDD
    cat $HOME/trades/fr11.trades.$YYYYMMDD >> $FILE; 

rsync --timeout=30 -avz  --quiet dvcinfra@10.23.200.52:/spare/local/ORSlogs/EUREX/*/trades.$YYYYMMDD   $HOME/trades/fr12.trades.$YYYYMMDD
cat $HOME/trades/fr12.trades.$YYYYMMDD >> $FILE; 

rsync --timeout=30 -avz  --quiet dvcinfra@10.23.200.53:/spare/local/ORSlogs/EUREX/*/trades.$YYYYMMDD   $HOME/trades/fr13.trades.$YYYYMMDD    
cat $HOME/trades/fr13.trades.$YYYYMMDD >> $FILE;

#    rsync --timeout=30 -avz  --quiet dvcinfra@10.23.200.53:/spare/local/ORSlogs/EUREX/NTAPROD3/trades.$YYYYMMDD   $HOME/trades/fr12.trades.$YYYYMMDD    
#    cat $HOME/trades/fr12.trades.$YYYYMMDD >> $FILE;


    rsync --timeout=30 -avz  --quiet dvcinfra@10.23.200.54:/spare/local/ORSlogs/EUREX/*/trades.$YYYYMMDD   $HOME/trades/fr14.trades.$YYYYMMDD
    cat $HOME/trades/fr14.trades.$YYYYMMDD >> $FILE; 

    rsync --timeout=30 -avz  --quiet dvcinfra@10.23.102.55:/spare/local/ORSlogs/EUREX/*/trades.$YYYYMMDD   $HOME/trades/fr15.trades.$YYYYMMDD
    cat $HOME/trades/fr15.trades.$YYYYMMDD >> $FILE; 

    rsync --timeout=30 -avz  --quiet dvcinfra@10.23.102.56:/spare/local/ORSlogs/EUREX/*/trades.$YYYYMMDD   $HOME/trades/fr16.trades.$YYYYMMDD
    cat $HOME/trades/fr16.trades.$YYYYMMDD >> $FILE; 


    #TMX server
    # scp -q dvcinfra@10.23.182.51:/spare/local/ORSlogs/TMX/BDMA/trades.$YYYYMMDD $HOME/trades
    # cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

    # scp -q dvcinfra@10.23.182.52:/spare/local/ORSlogs/TMX/BDMB/trades.$YYYYMMDD $HOME/trades
    # cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;


    #TMX server
    #This way we will have a backup to revert to the last known file if rsync --timeout=30 fails on the network
    #scp seems to be costly
	rsync --timeout=30 -avz  --quiet dvcinfra@10.23.182.51:/spare/local/ORSlogs/TMX/*/trades.$YYYYMMDD   $HOME/trades/tmx11.trades.$YYYYMMDD    
	cat $HOME/trades/tmx11.trades.$YYYYMMDD >> $FILE;

	rsync --timeout=30 -avz  --quiet dvcinfra@10.23.182.52:/spare/local/ORSlogs/TMX/*/trades.$YYYYMMDD   $HOME/trades/tmx12.trades.$YYYYMMDD
	cat $HOME/trades/tmx12.trades.$YYYYMMDD >> $FILE; 

    # Brazil
    # Add retail trades first
	rsync --timeout=30 -avz  --quiet dvcinfra@10.220.65.34:/spare/local/ORSlogs/BMFEP/RETAIL/trades.$YYYYMMDD   $HOME/trades/retail.trades.$YYYYMMDD
	cat $HOME/trades/retail.trades.$YYYYMMDD >> $FILE;

	rsync --timeout=30 -avz  --quiet dvcinfra@10.220.65.36:/spare/local/ORSlogs/BMFEP/MS001/trades.$YYYYMMDD   $HOME/trades/bmf15.trades.$YYYYMMDD
	cat $HOME/trades/bmf15.trades.$YYYYMMDD >> $FILE;

	rsync --timeout=30 -avz  --quiet dvcinfra@10.220.65.33:/spare/local/ORSlogs/BMFEP/MS002/trades.$YYYYMMDD   $HOME/trades/bmf13.trades.$YYYYMMDD    
	cat $HOME/trades/bmf13.trades.$YYYYMMDD >> $FILE;

	rsync --timeout=30 -avz  --quiet dvcinfra@10.220.65.34:/spare/local/ORSlogs/BMFEP/MS003/trades.$YYYYMMDD   $HOME/trades/bmf12.trades.$YYYYMMDD
	cat $HOME/trades/bmf12.trades.$YYYYMMDD >> $FILE;

	rsync --timeout=30 -avz  --quiet dvcinfra@10.220.65.38:/spare/local/ORSlogs/BMFEP/*/trades.$YYYYMMDD   $HOME/trades/bmf14.trades.$YYYYMMDD    
	cat $HOME/trades/bmf14.trades.$YYYYMMDD >> $FILE;

	rsync --timeout=30 -avz  --quiet dvcinfra@10.220.65.35:/spare/local/ORSlogs/BMFEQ/BMFEQ1/trades.$YYYYMMDD   $HOME/trades/bmfeq11.trades.$YYYYMMDD
	cat $HOME/trades/bmfeq11.trades.$YYYYMMDD >> $FILE;

	rsync --timeout=30 -avz  --quiet dvcinfra@10.220.65.35:/spare/local/ORSlogs/BMFEP/MS005/trades.$YYYYMMDD   $HOME/trades/bmf11_arb.trades.$YYYYMMDD
#cat $HOME/trades/bmf11_arb.trades.$YYYYMMDD >> $FILE;

    #LIFFE
    #rsync --timeout=30 -avz  --quiet dvcinfra@10.23.52.51:/spare/local/ORSlogs/LIFFE/4M0/trades.$YYYYMMDD   $HOME/trades/bsl11.trades.$YYYYMMDD
    #cat $HOME/trades/bsl11.trades.$YYYYMMDD >> $FILE;

    rsync --timeout=30 -avz  --quiet dvcinfra@10.23.52.51:/spare/local/ORSlogs/LIFFE/MSBSL1/trades.$YYYYMMDD   $HOME/trades/bsl11.ms.trades.$YYYYMMDD
    cat $HOME/trades/bsl11.ms.trades.$YYYYMMDD >> $FILE;

    rsync --timeout=30 -avz  --quiet dvcinfra@10.23.52.52:/spare/local/ORSlogs/LIFFE/MSBSL2/trades.$YYYYMMDD   $HOME/trades/bsl12.trades.$YYYYMMDD
    cat $HOME/trades/bsl12.trades.$YYYYMMDD >> $FILE;

    rsync --timeout=30 -avz  --quiet dvcinfra@10.23.52.53:/spare/local/ORSlogs/LIFFE/MSBSL3/trades.$YYYYMMDD   $HOME/trades/bsl13.trades.$YYYYMMDD
	cat $HOME/trades/bsl13.trades.$YYYYMMDD >> $FILE;

#ICE
	rsync --timeout=30 -avz  --quiet dvcinfra@10.23.52.51:/spare/local/ORSlogs/ICE/MSICE1/trades.$YYYYMMDD   $HOME/trades/bsl11.ice.trades.$YYYYMMDD
	cat $HOME/trades/bsl11.ice.trades.$YYYYMMDD >> $FILE;

        rsync --timeout=30 -avz  --quiet dvcinfra@10.23.52.53:/spare/local/ORSlogs/ICE/MSICE2/trades.$YYYYMMDD   $HOME/trades/bsl13.ice.trades.$YYYYMMDD
        cat $HOME/trades/bsl13.ice.trades.$YYYYMMDD >> $FILE;

#HKEX
         rsync --timeout=30 -avz  --quiet dvcinfra@10.152.224.145:/spare/local/ORSlogs/HKEX/FITGEN/trades.$YYYYMMDD   $HOME/trades/allhk11.trades.$YYYYMMDD
         cat $HOME/trades/allhk11.trades.$YYYYMMDD >> $FILE;

#OSE

        rsync --timeout=30 -avz  --quiet dvcinfra@10.134.210.182:/spare/local/ORSlogs/OSE/T2DVC22563/trades.$YYYYMMDD   $HOME/trades/alltok12.trades.$YYYYMMDD
        cat $HOME/trades/alltok12.trades.$YYYYMMDD >> $FILE;

        rsync --timeout=30 -avz  --quiet dvcinfra@10.134.210.184:/spare/local/ORSlogs/OSE/DVC22563/trades.$YYYYMMDD   $HOME/trades/alltok11.trades.$YYYYMMDD
        cat $HOME/trades/alltok11.trades.$YYYYMMDD >> $FILE;

##    rsync --timeout=30 -avz  --quiet dvcinfra@10.220.40.1:/spare/local/ORSlogs/BMFEP/XLIN68/trades.$YYYYMMDD   $HOME/trades/bmf13.trades.$YYYYMMDD    
##    cat $HOME/trades/bmf13.trades.$YYYYMMDD >> $FILE;
#MICEX
        rsync --timeout=30 -avz  --quiet dvcinfra@172.18.244.107:/spare/local/ORSlogs/MICEX/MICEXPROD01/trades.$YYYYMMDD $HOME/trades/allmicex11.trades.$YYYYMMDD
	cat $HOME/trades/allmicex11.trades.$YYYYMMDD >> $FILE;

#RTS
	rsync --timeout=30 -avz  --quiet dvcinfra@172.18.244.107:/spare/local/ORSlogs/RTS/FORTSPROD01/trades.$YYYYMMDD $HOME/trades/allrts11.trades.$YYYYMMDD
	cat $HOME/trades/allrts11.trades.$YYYYMMDD >> $FILE;

	rsync --timeout=30 -avz  --quiet dvcinfra@172.18.244.107:/spare/local/ORSlogs/RTSP2/RTSPLAZA/trades.$YYYYMMDD $HOME/trades/allrtsp211.trades.$YYYYMMDD
	cat $HOME/trades/allrtsp211.trades.$YYYYMMDD >> $FILE;

        rsync --timeout=30 -avz  --quiet dvcinfra@10.23.74.61:/spare/local/ORSlogs/CFE/MSCFE/trades.$YYYYMMDD   $HOME/trades/cfe11.trades.$YYYYMMDD
        cat $HOME/trades/cfe11.trades.$YYYYMMDD >> $FILE;

#NEWEDGE
        rsync --timeout=30 -avz  --quiet dvcinfra@10.23.74.53:/spare/local/ORSlogs/NEWEDGE/NEWEDGE/trades.$YYYYMMDD   $HOME/trades/newedge.trades.$YYYYMMDD
        cat $HOME/trades/newedge.trades.$YYYYMMDD >> $FILE;

#NSE
        rsync --timeout=30 -avz  --quiet dvcinfra@10.23.115.61:/spare/local/ORSlogs/NSE/MSFO/trades.$YYYYMMDD $HOME/trades/MSFO_allind11.trades.$YYYYMMDD
        cat $HOME/trades/MSFO_allind11.trades.$YYYYMMDD >> $FILE;

        rsync --timeout=30 -avz  --quiet dvcinfra@10.23.115.62:/spare/local/ORSlogs/NSE/MSCD/trades.$YYYYMMDD $HOME/trades/MSCD_allind12.trades.$YYYYMMDD
        cat $HOME/trades/MSCD_allind12.trades.$YYYYMMDD >> $FILE;

#ASX
      rsync --timeout=30 -avz  --quiet dvcinfra@10.23.43.51:/spare/local/ORSlogs/ASX/MSASX1/trades.$YYYYMMDD $HOME/trades/asx11.trades.$YYYYMMDD
      cat $HOME/trades/asx11.trades.$YYYYMMDD >> $FILE;

	clear;
	perl -w $PNL_SCRIPT 'C' $FILE | grep -v INVALI

	sleep 5;
    done

    rm -f $LOCKFILE;
else
    echo "$LOCKFILE present. Please delete";
fi
