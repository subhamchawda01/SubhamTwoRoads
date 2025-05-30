#!/bin/bash

#export NEW_GCC_LIB=/usr/local/lib
#export NEW_GCC_LIB64=/usr/local/lib64
#export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

FILE="$HOME/EODPnl/ALL_TRADES";
RETAIL_FILE="$HOME/EODPnl/RETAIL_TRADES";
PNL_SCRIPT="$HOME/infracore_install/scripts/see_ors_pnl.pl";
PNL_SCRIPT_RETAIL="$HOME/infracore_install/scripts/see_retail_pnl_nocolor.pl";
DUMP_OVN_PNL_SCRIPT="$HOME/infracore_install/scripts/see_ors_pnl.pl";
TMX_POS_COMPUTE="/home/dvcinfra/LiveExec/scripts/compute_tmx_positions.pl";

if [ ! -d $HOME/EODPnl ] ;
then
    mkdir $HOME/EODPnl;
fi

> $FILE;
> $RETAIL_FILE;

YYYYMMDD=$(date "+%Y%m%d");

if [ $# -ge 1 ] ;
then
    YYYYMMDD=$1;
fi

PREV_DATE=`$HOME/infracore_install/bin/calc_prev_week_day $YYYYMMDD` ;

GUI_TRADES_FILE=$HOME/EODPnl/gui_trades.$YYYYMMDD;
    if [ -e $GUI_TRADES_FILE ] ; then 
	cat $GUI_TRADES_FILE >> $FILE ;
    fi

    # scp -q dvcinfra@sdv-chi-srv11:/spare/local/ORSlogs/CME/HC0/trades.$YYYYMMDD $HOME/EODPnl
    # cat $HOME/EODPnl/trades.$YYYYMMDD >> $FILE; > $HOME/EODPnl/trades.$YYYYMMDD;

    # scp -q dvcinfra@sdv-chi-srv13:/spare/local/ORSlogs/CME/G52/trades.$YYYYMMDD $HOME/EODPnl
    # cat $HOME/EODPnl/trades.$YYYYMMDD >> $FILE; > $HOME/EODPnl/trades.$YYYYMMDD;

    # scp -q dvcinfra@sdv-chi-srv14:/spare/local/ORSlogs/CME/VD4/trades.$YYYYMMDD $HOME/EODPnl
    # cat $HOME/EODPnl/trades.$YYYYMMDD >> $FILE; > $HOME/EODPnl/trades.$YYYYMMDD;


    #CME servers
rsync -avz  --quiet dvcinfra@10.23.196.51:/spare/local/ORSlogs/CME/*/trades.$YYYYMMDD   $HOME/EODPnl/cme11.trades.$YYYYMMDD    
cat $HOME/EODPnl/cme11.trades.$YYYYMMDD >> $FILE;

rsync -avz  --quiet dvcinfra@10.23.196.52:/spare/local/ORSlogs/CME/*/trades.$YYYYMMDD   $HOME/EODPnl/cme12.trades.$YYYYMMDD    
cat $HOME/EODPnl/cme12.trades.$YYYYMMDD >> $FILE;

rsync -avz  --quiet dvcinfra@10.23.196.53:/spare/local/ORSlogs/CME/*/trades.$YYYYMMDD   $HOME/EODPnl/cme13.trades.$YYYYMMDD    
cat $HOME/EODPnl/cme13.trades.$YYYYMMDD >> $FILE;

rsync -avz  --quiet dvcinfra@10.23.196.54:/spare/local/ORSlogs/CME/*/trades.$YYYYMMDD   $HOME/EODPnl/cme14.trades.$YYYYMMDD    
cat $HOME/EODPnl/cme14.trades.$YYYYMMDD >> $FILE;

rsync -avz  --quiet dvcinfra@10.23.82.55:/spare/local/ORSlogs/CME/*/trades.$YYYYMMDD   $HOME/EODPnl/cme15.trades.$YYYYMMDD    
cat $HOME/EODPnl/cme15.trades.$YYYYMMDD >> $FILE;

rsync -avz  --quiet dvcinfra@10.23.82.56:/spare/local/ORSlogs/CME/*/trades.$YYYYMMDD   $HOME/EODPnl/cme16.trades.$YYYYMMDD    
cat $HOME/EODPnl/cme16.trades.$YYYYMMDD >> $FILE;

    #EUREX server
    # scp -q dvcinfra@sdv-fr2-srv11:/spare/local/ORSlogs/EUREX/UTE002/trades.$YYYYMMDD $HOME/EODPnl
    # cat $HOME/EODPnl/trades.$YYYYMMDD >> $FILE; > $HOME/EODPnl/trades.$YYYYMMDD;

    # scp -q dvcinfra@sdv-fr2-srv14:/spare/local/ORSlogs/EUREX/UTE001/trades.$YYYYMMDD $HOME/EODPnl
    # cat $HOME/EODPnl/trades.$YYYYMMDD >> $FILE; > $HOME/EODPnl/trades.$YYYYMMDD;

    # scp -q dvcinfra@sdv-fr2-srv13:/spare/local/ORSlogs/EUREX/UTE001/trades.$YYYYMMDD $HOME/EODPnl
    # cat $HOME/EODPnl/trades.$YYYYMMDD >> $FILE; > $HOME/EODPnl/trades.$YYYYMMDD;


    #EUREX server
    #This way we will have a backup to revert to the last known file if rsync fails on the network
    #scp seems to be costly
rsync -avz  --quiet dvcinfra@10.23.102.55:/spare/local/ORSlogs/EUREX/*/trades.$YYYYMMDD   $HOME/EODPnl/fr15.trades.$YYYYMMDD
cat $HOME/EODPnl/fr15.trades.$YYYYMMDD >> $FILE; 

rsync -avz  --quiet dvcinfra@10.23.200.54:/spare/local/ORSlogs/EUREX/*/trades.$YYYYMMDD   $HOME/EODPnl/fr14.trades.$YYYYMMDD    
cat $HOME/EODPnl/fr14.trades.$YYYYMMDD >> $FILE;

rsync -avz  --quiet dvcinfra@10.23.102.56:/spare/local/ORSlogs/EUREX/*/trades.$YYYYMMDD   $HOME/EODPnl/fr16.trades.$YYYYMMDD
cat $HOME/EODPnl/fr16.trades.$YYYYMMDD >> $FILE; 

rsync -avz  --quiet dvcinfra@10.23.200.53:/spare/local/ORSlogs/EUREX/*/trades.$YYYYMMDD   $HOME/EODPnl/fr13.trades.$YYYYMMDD    
cat $HOME/EODPnl/fr13.trades.$YYYYMMDD >> $FILE;


    #TMX server
    # scp -q dvcinfra@10.23.182.51:/spare/local/ORSlogs/TMX/BDMA/trades.$YYYYMMDD $HOME/EODPnl
    # cat $HOME/EODPnl/trades.$YYYYMMDD >> $FILE; > $HOME/EODPnl/trades.$YYYYMMDD;

    # scp -q dvcinfra@10.23.182.52:/spare/local/ORSlogs/TMX/BDMB/trades.$YYYYMMDD $HOME/EODPnl
    # cat $HOME/EODPnl/trades.$YYYYMMDD >> $FILE; > $HOME/EODPnl/trades.$YYYYMMDD;


    #TMX server
    #This way we will have a backup to revert to the last known file if rsync fails on the network
    #scp seems to be costly
rsync -avz  --quiet dvcinfra@10.23.182.51:/spare/local/ORSlogs/TMX/*/trades.$YYYYMMDD   $HOME/EODPnl/tmx11.trades.$YYYYMMDD    
cat $HOME/EODPnl/tmx11.trades.$YYYYMMDD >> $FILE;

#rsync -avz  --quiet dvcinfra@10.23.182.52:/spare/local/ORSlogs/TMX/BDMB/trades.$YYYYMMDD   $HOME/EODPnl/tmx12.trades.$YYYYMMDD
#cat $HOME/EODPnl/tmx12.trades.$YYYYMMDD >> $FILE; 

rsync --timeout=30 -avz  --quiet dvcinfra@10.23.182.52:/spare/local/ORSlogs/TMX/*/trades.$YYYYMMDD   $HOME/trades/tmx12.trades.$YYYYMMDD
cat $HOME/trades/tmx12.trades.$YYYYMMDD >> $FILE; 


# Brazil


#    rsync -avz  --quiet dvcinfra@10.23.23.12:/spare/local/ORSlogs/BMFEP/XALP0055/trades.$YYYYMMDD   $HOME/EODPnl/bmf12.trades.$YYYYMMDD
rsync -avz  --quiet dvcinfra@10.23.23.12:/spare/local/ORSlogs/BMFEP/MS002/trades.$YYYYMMDD   $HOME/EODPnl/bmf12.trades.$YYYYMMDD   
cat $HOME/EODPnl/bmf12.trades.$YYYYMMDD >> $FILE;

rsync -avz  --quiet dvcinfra@10.23.23.12:/spare/local/ORSlogs/BMFEP/RETAIL/trades.$YYYYMMDD   $HOME/EODPnl/bmf11.trades.retail.$YYYYMMDD   
cat $HOME/EODPnl/bmf11.trades.retail.$YYYYMMDD >> $FILE;
cat $HOME/EODPnl/bmf11.trades.retail.$YYYYMMDD >> $RETAIL_FILE;

rsync -avz  --quiet dvcinfra@10.23.23.13:/spare/local/ORSlogs/BMFEP/MS002/trades.$YYYYMMDD   $HOME/EODPnl/bmf13.trades.$YYYYMMDD    
cat $HOME/EODPnl/bmf13.trades.$YYYYMMDD >> $FILE;

rsync -avz  --quiet dvcinfra@10.23.23.14:/spare/local/ORSlogs/BMFEP/MS004/trades.$YYYYMMDD   $HOME/EODPnl/bmf14.trades.$YYYYMMDD    
cat $HOME/EODPnl/bmf14.trades.$YYYYMMDD >> $FILE;

#    rsync -avz --quiet dvcinfra@10.23.23.11:/home/spare/local/ORSlogs/BMFEP/XALP004972/trades.$YYYYMMDD $HOME/EODPnl/bmf11.trades.$YYYYMMDD
rsync -avz --quiet dvcinfra@10.23.23.11:/spare/local/ORSlogs/BMFEP/MS001/trades.$YYYYMMDD $HOME/EODPnl/bmf11.trades.$YYYYMMDD
cat $HOME/EODPnl/bmf11.trades.$YYYYMMDD >> $FILE;

rsync -avz --quiet dvcinfra@10.23.23.15:/spare/local/ORSlogs/BMFEP/MS001/trades.$YYYYMMDD $HOME/EODPnl/bmf15.trades.$YYYYMMDD
cat $HOME/EODPnl/bmf15.trades.$YYYYMMDD >> $FILE;

#rsync -avz  --quiet dvcinfra@10.23.23.11:/spare/local/ORSlogs/BMFEQ/BMFEQ1/trades.$YYYYMMDD   $HOME/trades/bmfeq11.trades.$YYYYMMDD
#cat $HOME/trades/bmfeq11.trades.$YYYYMMDD >> $FILE;

rsync -avz  --quiet dvcinfra@10.23.23.11:/spare/local/ORSlogs/BMFEP/MS005/trades.$YYYYMMDD   $HOME/trades/bmf11_arb.trades.$YYYYMMDD
cat $HOME/trades/bmf11_arb.trades.$YYYYMMDD >> $FILE;

#LIFFE
rsync -avz  --quiet dvcinfra@10.23.52.51:/spare/local/ORSlogs/LIFFE/4M0/trades.$YYYYMMDD   $HOME/EODPnl/bsl11.trades.$YYYYMMDD
cat $HOME/EODPnl/bsl11.trades.$YYYYMMDD >> $FILE;

rsync -avz  --quiet dvcinfra@10.23.52.51:/spare/local/ORSlogs/LIFFE/MSBSL1/trades.$YYYYMMDD   $HOME/EODPnl/bsl11.ms.trades.$YYYYMMDD
cat $HOME/EODPnl/bsl11.ms.trades.$YYYYMMDD >> $FILE;

rsync -avz  --quiet dvcinfra@10.23.52.52:/spare/local/ORSlogs/LIFFE/MSBSL2/trades.$YYYYMMDD   $HOME/EODPnl/bsl12.trades.$YYYYMMDD
cat $HOME/EODPnl/bsl12.trades.$YYYYMMDD >> $FILE;

rsync -avz  --quiet dvcinfra@10.23.52.53:/spare/local/ORSlogs/LIFFE/MSBSL3/trades.$YYYYMMDD   $HOME/EODPnl/bsl13.trades.$YYYYMMDD
cat $HOME/EODPnl/bsl13.trades.$YYYYMMDD >> $FILE;

#ICE
rsync -avz  --quiet dvcinfra@10.23.52.51:/spare/local/ORSlogs/ICE/MSICE1/trades.$YYYYMMDD   $HOME/EODPnl/bsl11.ice.trades.$YYYYMMDD
cat $HOME/EODPnl/bsl11.ice.trades.$YYYYMMDD >> $FILE;

rsync -avz  --quiet dvcinfra@10.23.52.53:/spare/local/ORSlogs/ICE/MSICE2/trades.$YYYYMMDD   $HOME/EODPnl/bsl13.ice.trades.$YYYYMMDD
cat $HOME/EODPnl/bsl13.ice.trades.$YYYYMMDD >> $FILE;

#HKEX
rsync -avz  --quiet dvcinfra@10.152.224.145:/spare/local/ORSlogs/HKEX/FITGEN/trades.$YYYYMMDD   $HOME/EODPnl/hk12.trades.$YYYYMMDD
cat $HOME/EODPnl/hk12.trades.$YYYYMMDD >> $FILE;

#rsync -avz  --quiet dvcinfra@10.152.224.146:/spare/local/ORSlogs/HKEX/FITGEN/trades.$YYYYMMDD   $HOME/EODPnl/hk11.trades.$YYYYMMDD
#cat $HOME/EODPnl/hk11.trades.$YYYYMMDD >> $FILE;

#OSE

rsync -avz  --quiet dvcinfra@10.134.210.184:/spare/local/ORSlogs/OSE/DVC22563/trades.$YYYYMMDD   $HOME/EODPnl/alltok11.trades.$YYYYMMDD 
cat $HOME/EODPnl/alltok11.trades.$YYYYMMDD >> $FILE;

rsync -avz  --quiet dvcinfra@10.134.210.182:/spare/local/ORSlogs/OSE/T2DVC22563/trades.$YYYYMMDD   $HOME/EODPnl/alltok12.trades.$YYYYMMDD 
cat $HOME/EODPnl/alltok12.trades.$YYYYMMDD >> $FILE;
#MICEX
rsync -avz  --quiet dvcinfra@172.18.244.107:/spare/local/ORSlogs/MICEX/MICEXPROD01/trades.$YYYYMMDD $HOME/EODPnl/allmicex11.trades.$YYYYMMDD
cat $HOME/EODPnl/allmicex11.trades.$YYYYMMDD >> $FILE;

#RTS
rsync -avz  --quiet dvcinfra@172.18.244.107:/spare/local/ORSlogs/RTS/FORTSPROD01/trades.$YYYYMMDD $HOME/EODPnl/allrts11.trades.$YYYYMMDD
cat $HOME/EODPnl/allrts11.trades.$YYYYMMDD >> $FILE;

rsync -avz  --quiet dvcinfra@10.23.74.61:/spare/local/ORSlogs/CFE/MSCFE/trades.$YYYYMMDD $HOME/EODPnl/cfe.trades.$YYYYMMDD        
cat $HOME/EODPnl/cfe.trades.$YYYYMMDD >> $FILE ; 

#NEWEDGE
rsync -avz  --quiet dvcinfra@10.23.74.53:/spare/local/ORSlogs/NEWEDGE/NEWEDGE/trades.$YYYYMMDD $HOME/EODPnl/newedge.trades.$YYYYMMDD
cat $HOME/EODPnl/newedge.trades.$YYYYMMDD >> $FILE;

#NSE
rsync -avz  --quiet dvcinfra@10.23.115.61:/spare/local/ORSlogs/NSE/MSFO/trades.$YYYYMMDD $HOME/EODPnl/MSFO_allind11.trades.$YYYYMMDD
cat $HOME/EODPnl/MSFO_allind11.trades.$YYYYMMDD >> $FILE;

rsync -avz  --quiet dvcinfra@10.23.115.62:/spare/local/ORSlogs/NSE/MSCD/trades.$YYYYMMDD $HOME/EODPnl/MSCD_allind12.trades.$YYYYMMDD
cat $HOME/EODPnl/MSCD_allind12.trades.$YYYYMMDD >> $FILE;

#ASX
rsync -avz  --quiet dvcinfra@10.23.43.51:/spare/local/ORSlogs/ASX/MSASX1/trades.$YYYYMMDD $HOME/EODPnl/asx11.trades.$YYYYMMDD
cat $HOME/EODPnl/asx11.trades.$YYYYMMDD >> $FILE;

OUT_FILE="/apps/data/MFGlobalTrades/EODPnl/ors_pnls_$YYYYMMDD.txt";

if [ $# -eq 3 ] ;
then
	OUT_FILE="/apps/data/MFGlobalTrades/InteradayPnl/ors_pnls_$YYYYMMDD.txt";
fi

if [ $# -eq 0 ] ;
then
	perl -w $PNL_SCRIPT 'R' $FILE > /apps/data/MFGlobalTrades/EODPnl/ors_pnls_$YYYYMMDD".txt"
else
    perl -w $PNL_SCRIPT 'R' $FILE $YYYYMMDD > ${OUT_FILE};
fi

if [ "$#" -eq 2 ]; then
    perl -w $DUMP_OVN_PNL_SCRIPT 'R' $FILE $YYYYMMDD 1 > /spare/local/files/EODPositions/overnight_pnls_"$YYYYMMDD"_tmp.txt 2>/dev/null 
    perl -w $TMX_POS_COMPUTE /spare/local/files/EODPositions/overnight_pnls_"$YYYYMMDD"_tmp.txt > /spare/local/files/EODPositions/overnight_pnls_"$YYYYMMDD"_tmp2.txt 2>/dev/null; 
    cp /spare/local/files/EODPositions/overnight_pnls_"$YYYYMMDD"_tmp2.txt /spare/local/files/EODPositions/overnight_pnls_"$YYYYMMDD".txt    
    rm /spare/local/files/EODPositions/overnight_pnls_"$YYYYMMDD"_tmp2.txt 2>/dev/null;
fi
cat /apps/data/MFGlobalTrades/EODPnl/ors_pnls_$YYYYMMDD".txt"

    #sync to file-server 
scp /apps/data/MFGlobalTrades/EODPnl/ors_pnls_$YYYYMMDD".txt" dvcinfra@10.23.74.40:/apps/data/MFGlobalTrades/EODPnl/ >/dev/null 2>/dev/null
