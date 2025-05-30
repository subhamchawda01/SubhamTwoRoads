#!/bin/bash

#export NEW_GCC_LIB=/usr/local/lib
#export NEW_GCC_LIB64=/usr/local/lib64
#export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

FILE="$HOME/EODPnl/ALL_EQ_TRADES";
PNL_SCRIPT="$HOME/infracore_install/scripts/see_ors_equities_pnl_nocolor.pl";
DUMP_OVN_PNL_SCRIPT="/home/dvcinfra/LiveExec/scripts/see_ors_pnl.pl";

if [ ! -d $HOME/EODPnl ] ;
then
    mkdir $HOME/EODPnl;
fi

> $FILE;

YYYYMMDD=$(date "+%Y%m%d");

if [ $# -eq 1 ] ;
then
    YYYYMMDD=$1;
fi

rsync -avz  --quiet dvcinfra@10.23.23.12:/spare/local/ORSlogs/BMFEQ/BMFEQ1/trades.$YYYYMMDD   $HOME/EODPnl/bmfeq11.trades.$YYYYMMDD
cat $HOME/EODPnl/bmfeq11.trades.$YYYYMMDD >> $FILE;

if [ $# -eq 1 ] ;
then
    perl -w $PNL_SCRIPT $FILE $YYYYMMDD > /apps/data/MFGlobalTrades/EODPnl/ors_equities_pnls_$YYYYMMDD".txt"
else
    perl -w $PNL_SCRIPT $FILE > /apps/data/MFGlobalTrades/EODPnl/ors_equities_pnls_$YYYYMMDD".txt"
fi

if [ "$#" -eq 2 ]; then
    perl -w $DUMP_OVN_PNL_SCRIPT 'C' $FILE $YYYYMMDD 1 > "/spare/local/files/EODPositions/overnight_equities_pnls.txt" 2>/dev/null
    cp /spare/local/files/EODPositions/overnight_equities_pnls.txt /spare/local/files/EODPositions/overnight_equities_pnls_read.txt
fi

#cat /apps/data/MFGlobalTrades/EODPnl/ors_equities_pnls_$YYYYMMDD".txt"

#sync to file-server 
chmod 666 /apps/data/MFGlobalTrades/EODPnl/ors_equities_pnls_$YYYYMMDD".txt" ;
scp /apps/data/MFGlobalTrades/EODPnl/ors_equities_pnls_$YYYYMMDD".txt" dvcinfra@10.23.74.40:/apps/data/MFGlobalTrades/EODPnl/ >/dev/null 2>/dev/null

#Print Mail output
YYYY=${YYYYMMDD:0:4}
MM=${YYYYMMDD:4:2}
DD=${YYYYMMDD:6:2}

while read line
do
  if [[ ! "$line" =~ "POSITION" ]]; then
    echo "$line"
    continue
  fi
  #Get stock symbol
  stock=`echo $line | cut -d '|' -f2 | tr -d ' '`;
  market_vol=`echo $line | /home/dvcinfra/infracore_install/bin/mds_log_reader NTP /NAS1/data/PUMALoggedData/BRZ/$YYYY/$MM/$DD/${stock}_$YYYYMMDD.gz | grep "Trade_Size:" | awk '{sum+=$2} END{print sum}'`
  our_vol=`echo $line | cut -d'|' -f5 | cut -d':' -f 2| tr -d ' '`
  echo $line | awk -v ov=$our_vol -v mvol=$market_vol '{printf "%s  MV: %d | Market Share: %0.5f\n", $0, mvol, ov/mvol}' ;
done < /NAS1/data/MFGlobalTrades/EODPnl/ors_equities_pnls_$YYYYMMDD".txt" > /apps/data/MFGlobalTrades/EODPnl/ors_equities_pnls_$YYYYMMDD.txt ;

ssh -T dvctrader@localhost <<EOF
  our_notional=\`for i in \$(ls /NAS1/logs/QueryTrades/$YYYY/$MM/$DD/trades.$YYYYMMDD.40001?) ; do /home/dvctrader/basetrade/ModelScripts/get_max_notional_risk_from_tradefile.pl \$i 1 ; done 2>/dev/null | grep TOTAL | awk '{sum+=\$2} END{printf "%0.5f\n",sum;}'\`
                        
  total_notional=\`for stock in \$(cat /spare/local/files/BMF/stock_list) ; do /home/dvctrader/basetrade_install/bin/mkt_trade_logger SIM \$stock $YYYYMMDD BMF_EQ | grep Trade | awk '{print \$5*\$7}' ; done 2>/dev/null | awk '{sum+=\$1}END{printf "%0.5f\n",sum}'\`
                         
  echo "" | awk -v our=\$our_notional -v total=\$total_notional '{printf "Our notional: %s | Total Market Notional: %s | Fraction: %0.5f\n",our,total,our/total}' >> /apps/data/MFGlobalTrades/EODPnl/ors_equities_pnls_$YYYYMMDD.txt 

  rb=\`/home/dvctrader/basetrade/scripts/get_equities_rebate.pl \$our_notional \`
  if [[ \$rb >  0 ]] ; then  echo "REBATE: \$rb" ;  fi >> /apps/data/MFGlobalTrades/EODPnl/ors_equities_pnls_$YYYYMMDD.txt
EOF

cat /apps/data/MFGlobalTrades/EODPnl/ors_equities_pnls_$YYYYMMDD".txt"

scp /apps/data/MFGlobalTrades/EODPnl/ors_equities_pnls_$YYYYMMDD".txt" dvcinfra@10.23.74.40:/apps/data/MFGlobalTrades/EODPnl/ >/dev/null 2>/dev/null

/home/dvcinfra/infracore/scripts/see_rolling_avg_pnl.pl 
