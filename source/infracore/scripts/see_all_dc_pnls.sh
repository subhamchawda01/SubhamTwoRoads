#!/bin/bash

FORMAT="script [YYYYMMDD]";

LOCKFILE=$HOME/locks/seealldc.lock

if [ ! -e $LOCKFILE ] ; then
touch $LOCKFILE;
FILE="$HOME/trades/ALL_DC_TRADES";
EACH_FILE="$HOME/trades/EXCH_DC_TRADES"
PNL_SCRIPT="$HOME/LiveExec/scripts/see_ors_dc_pnl.pl";
if [ ! -d $HOME/trades ] ;
then
    mkdir $HOME/trades;
fi

while [ true ]
do
   > $FILE
   > $EACH_FILE
   YYYYMMDD=$(date --date='1 hour 30 minutes' "+%Y%m%d");
   
#liffe
   trade_file="/spare/local/ORSlogs/LIFFE/EJG9/trades.$YYYYMMDD"
   rsync -avz --timeout=60 --quiet dvcinfra@10.23.52.52:$trade_file $EACH_FILE
   cat $EACH_FILE >> $FILE	
   > $EACH_FILE
   
   trade_file="/spare/local/ORSlogs/LIFFE/JG9/trades.$YYYYMMDD"
   rsync -avz --timeout=60 --quiet dvcinfra@10.23.52.53:$trade_file $EACH_FILE
   cat $EACH_FILE >> $FILE      
   > $EACH_FILE   
   
#ICE
   trade_file="/spare/local/ORSlogs/ICE/ICEDC/trades.$YYYYMMDD"
   rsync -avz --timeout=60 --quiet dvcinfra@10.23.52.51:$trade_file $EACH_FILE
   cat $EACH_FILE >> $FILE
   > $EACH_FILE

   trade_file="/spare/local/ORSlogs/ICE/ICEDC1/trades.$YYYYMMDD"
   rsync -avz --timeout=60 --quiet dvcinfra@10.23.52.53:$trade_file $EACH_FILE
   cat $EACH_FILE >> $FILE
   > $EACH_FILE   
   
#CFE
   trade_file="/spare/local/ORSlogs/CFE/FFDVC/trades.$YYYYMMDD"
   rsync -avz --timeout=60 --quiet dvcinfra@10.23.74.61:$trade_file $EACH_FILE
   cat $EACH_FILE >> $FILE
   > $EACH_FILE

#BMF
   GUI_TRADES_FILE=$HOME/trades/bmf_rodrigo_trades.$YYYYMMDD
   if [ -e $GUI_TRADES_FILE ] ; then
        cat $GUI_TRADES_FILE >> $FILE ;
   fi

   trade_file="/spare/local/ORSlogs/BMFEP/RETAIL/trades.$YYYYMMDD"
   rsync -avz --timeout=60 --quiet dvcinfra@10.220.65.34:$trade_file $EACH_FILE
   cat $EACH_FILE >> $FILE
   > $EACH_FILE

   trade_file="/spare/local/ORSlogs/BMFEP/DBRP0004/trades.$YYYYMMDD"
   rsync -avz --timeout=60 --quiet dvcinfra@10.220.65.34:$trade_file $EACH_FILE
   cat $EACH_FILE >> $FILE
   > $EACH_FILE

#RTS
   trade_file="/spare/local/ORSlogs/RTSDC/FORTSDC/trades.$YYYYMMDD"
   rsync -avz --timeout=60 --quiet dvcinfra@172.18.244.107:$trade_file $EACH_FILE
   cat $EACH_FILE >> $FILE
   > $EACH_FILE

#MICEX
   trade_file="/spare/local/ORSlogs/MICEXDC/MICEXDC/trades.$YYYYMMDD"
   rsync -avz --timeout=60 --quiet dvcinfra@172.18.244.107:$trade_file $EACH_FILE
   cat $EACH_FILE >> $FILE
   > $EACH_FILE

#TMX
  trade_file="/spare/local/ORSlogs/TMXATR/BDMATR/atr_trades.$YYYYMMDD"
   rsync -avz --timeout=60 --quiet dvcinfra@10.23.182.52:$trade_file $EACH_FILE
   cat $EACH_FILE >> $FILE
   > $EACH_FILE

#ASX
      trade_file="/spare/local/ORSlogs/ASX/ASXDC/trades.$YYYYMMDD"
   rsync -avz --timeout=60 --quiet dvcinfra@10.23.43.52:$trade_file $EACH_FILE
   cat $EACH_FILE >> $FILE
   > $EACH_FILE

   time_now_=`date +"%H%M%S"` ;

   if [ "$time_now_" -gt "220000" -a "$time_now_" -lt "221000" ] ; then
      perl -w $PNL_SCRIPT 'R' $FILE $YYYYMMDD 0 > "/spare/local/files/EODPnl/ors_dc_pnls_"$YYYYMMDD".txt" 2>/dev/null;
      perl -w $PNL_SCRIPT 'R' $FILE $YYYYMMDD 1 > "/spare/local/files/EODPositions/overnight_dc_pnls_"$YYYYMMDD".txt" 2>/dev/null;
      exit 0;
   else
      perl -w $PNL_SCRIPT 'C' $FILE $YYYYMMDD 0   
      sleep 30;
   fi

done
rm -f $LOCKFILE;
else
echo "$LOCKFILE present. Please delete";
fi

