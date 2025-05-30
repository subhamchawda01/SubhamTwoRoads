#!/bin/bash

OPEN_POSITION_FILE="/tmp/trades/ALL_OPEN_POSITIONS" ;

FILE="/tmp/trades/ALL_TRADES";
PNL_SCRIPT="/home/dvcinfra/LiveExec/scripts/mfg_ors_pnl_csv.pl";
REBATE_SCRIPT="/home/dvcinfra/LiveExec/scripts/per_product_accumulated_rebate.pl";


YYYYMMDD=$(cat /tmp/YESTERDAY_DATE);

PREVDAY_EXEC=/home/dvcinfra/LiveExec/bin/calc_prev_week_day 

OSE_DAY=`$PREVDAY_EXEC $YYYYMMDD` ;
MOS_DAY=`$PREVDAY_EXEC $YYYYMMDD` ;

OSE_RECONCILE_SCRIPT="/home/dvcinfra/LiveExec/scripts/reconcile_OSE_ORS_mfg_pnls.sh";
MOS_RECONCILE_SCRIPT="/home/dvcinfra/LiveExec/scripts/reconcile_MOS_ORS_bcs_pnls.sh" 

> $FILE;

#OSE reconcile 
`$OSE_RECONCILE_SCRIPT $OSE_DAY`
`$MOS_RECONCILE_SCRIPT $MOS_DAY`

if [ ! -d /tmp/trades ] ;
then
    mkdir /tmp/trades;
fi

> $FILE;

cat $OPEN_POSITION_FILE >> $FILE ;

## CME START ##
scp -q dvcinfra@sdv-chi-srv11:/spare/local/ORSlogs/CME/MSCME1/trades.$YYYYMMDD /tmp/trades
cat /tmp/trades/trades.$YYYYMMDD > $FILE; > /tmp/trades/trades.$YYYYMMDD;

scp -q dvcinfra@sdv-chi-srv12:/spare/local/ORSlogs/CME/MSCME2/trades.$YYYYMMDD /tmp/trades
cat /tmp/trades/trades.$YYYYMMDD >> $FILE; > /tmp/trades/trades.$YYYYMMDD;

scp -q dvcinfra@sdv-chi-srv13:/spare/local/ORSlogs/CME/MSCME3/trades.$YYYYMMDD /tmp/trades
cat /tmp/trades/trades.$YYYYMMDD >> $FILE; > /tmp/trades/trades.$YYYYMMDD;

scp -q dvcinfra@sdv-chi-srv14:/spare/local/ORSlogs/CME/MSCME4/trades.$YYYYMMDD /tmp/trades
cat /tmp/trades/trades.$YYYYMMDD >> $FILE; > /tmp/trades/trades.$YYYYMMDD;
## CME END ##

## EUREX START ##
scp -q dvcinfra@sdv-fr2-srv11:/spare/local/ORSlogs/EUREX/NTAPROD*/trades.$YYYYMMDD /tmp/trades
cat /tmp/trades/trades.$YYYYMMDD >> $FILE; > /tmp/trades/trades.$YYYYMMDD;

scp -q dvcinfra@sdv-fr2-srv12:/spare/local/ORSlogs/EUREX/NTAPROD*/trades.$YYYYMMDD /tmp/trades
cat /tmp/trades/trades.$YYYYMMDD >> $FILE; > /tmp/trades/trades.$YYYYMMDD;

scp -q dvcinfra@sdv-fr2-srv13:/spare/local/ORSlogs/EUREX/NTAPROD*/trades.$YYYYMMDD /tmp/trades
cat /tmp/trades/trades.$YYYYMMDD >> $FILE; > /tmp/trades/trades.$YYYYMMDD;

scp -q dvcinfra@sdv-fr2-srv14:/spare/local/ORSlogs/EUREX/NTAPROD*/trades.$YYYYMMDD /tmp/trades
cat /tmp/trades/trades.$YYYYMMDD >> $FILE; > /tmp/trades/trades.$YYYYMMDD;
## EUREX END ##

## TMX START ##
scp -q dvcinfra@10.23.182.51:/spare/local/ORSlogs/TMX/MSTR1/trades.$YYYYMMDD /tmp/trades
cat /tmp/trades/trades.$YYYYMMDD >> $FILE; > /tmp/trades/trades.$YYYYMMDD;

scp -q dvcinfra@10.23.182.52:/spare/local/ORSlogs/TMX/*/trades.$YYYYMMDD /tmp/trades
cat /tmp/trades/trades.$YYYYMMDD >> $FILE; > /tmp/trades/trades.$YYYYMMDD;
## TMX END ##

## BMF START ##
#scp -q dvcinfra@10.23.23.11:/spare/local/ORSlogs/BMFEP/XALP0037/trades.$YYYYMMDD /tmp/trades
#cat /tmp/trades/trades.$YYYYMMDD >> $FILE; > /tmp/trades/trades.$YYYYMMDD;

scp -q dvcinfra@10.23.23.11:/spare/local/ORSlogs/BMFEP/MS001/trades.$YYYYMMDD /tmp/trades
cat /tmp/trades/trades.$YYYYMMDD >> $FILE; > /tmp/trades/trades.$YYYYMMDD;

scp -q dvcinfra@10.23.23.12:/spare/local/ORSlogs/BMFEP/MS002/trades.$YYYYMMDD /tmp/trades
cat /tmp/trades/trades.$YYYYMMDD >> $FILE; > /tmp/trades/trades.$YYYYMMDD;

scp -q dvcinfra@10.23.23.13:/spare/local/ORSlogs/BMFEP/MS003/trades.$YYYYMMDD /tmp/trades
cat /tmp/trades/trades.$YYYYMMDD >> $FILE; > /tmp/trades/trades.$YYYYMMDD;

scp -q dvcinfra@10.23.23.14:/spare/local/ORSlogs/BMFEP/MS004/trades.$YYYYMMDD /tmp/trades
cat /tmp/trades/trades.$YYYYMMDD >> $FILE; > /tmp/trades/trades.$YYYYMMDD;

scp -q dvcinfra@10.23.23.15:/spare/local/ORSlogs/BMFEP/MS005/trades.$YYYYMMDD /tmp/trades
cat /tmp/trades/trades.$YYYYMMDD >> $FILE; > /tmp/trades/trades.$YYYYMMDD;

#scp -q dvcinfra@10.23.23.11:/spare/local/ORSlogs/BMFEP/*/trades.$YYYYMMDD /tmp/trades/
#cat /tmp/trades/trades.$YYYYMMDD >> $FILE; > /tmp/trades/trades.$YYYYMMDD;

#scp -q dvcinfra@10.220.40.1:/spare/local/ORSlogs/BMFEP/XLIN68/trades.$YYYYMMDD /tmp/trades
#cat /tmp/trades/trades.$YYYYMMDD >> $FILE; > /tmp/trades/trades.$YYYYMMDD;
## BMF END ##

## LIFFE START ##
scp -q dvcinfra@10.23.52.51:/spare/local/ORSlogs/LIFFE/MSBSL1/trades.$YYYYMMDD /tmp/trades
cat /tmp/trades/trades.$YYYYMMDD >> $FILE; > /tmp/trades/trades.$YYYYMMDD;

scp -q dvcinfra@10.23.52.52:/spare/local/ORSlogs/LIFFE/*/trades.$YYYYMMDD /tmp/trades
cat /tmp/trades/trades.$YYYYMMDD >> $FILE; > /tmp/trades/trades.$YYYYMMDD;

scp -q dvcinfra@10.23.52.53:/spare/local/ORSlogs/LIFFE/*/trades.$YYYYMMDD /tmp/trades
cat /tmp/trades/trades.$YYYYMMDD >> $FILE; > /tmp/trades/trades.$YYYYMMDD;
## LIFFE END ##

## HK START ##
scp -q dvcinfra@10.152.224.145:/spare/local/ORSlogs/HKEX/FITGEN/trades.$YYYYMMDD /tmp/trades
cat /tmp/trades/trades.$YYYYMMDD >> $FILE; > /tmp/trades/trades.$YYYYMMDD;

scp -q dvcinfra@10.152.224.146:/spare/local/ORSlogs/HKEX/*/trades.$YYYYMMDD /tmp/trades
cat /tmp/trades/trades.$YYYYMMDD >> $FILE; > /tmp/trades/trades.$YYYYMMDD;
## HK END ##

#MICEX
scp -q dvcinfra@172.18.244.107:/spare/local/ORSlogs/MICEX/MICEXPROD01/trades.$YYYYMMDD /tmp/trades
cat /tmp/trades/trades.$YYYYMMDD >> $FILE; > /tmp/trades/trades.$YYYYMMDD;

#RTS
scp -q dvcinfra@172.18.244.107:/spare/local/ORSlogs/RTS/FORTSPROD01/trades.$YYYYMMDD /tmp/trades
cat /tmp/trades/trades.$YYYYMMDD >> $FILE; > /tmp/trades/trades.$YYYYMMDD;

> /tmp/ORS_pnls_diffs.$YYYYMMDD;
perl -w $PNL_SCRIPT $FILE $YYYYMMDD > /tmp/ORS_pnls_diffs.$YYYYMMDD;

REBATE_DATA_FILE=/apps/data/MFGlobalTrades/REBATE/rebate_amount_$YYYYMMDD;
>$REBATE_DATA_FILE ;

REBATE_LIST_FILE=/apps/data/MFGlobalTrades/REBATE/rebate_shortcode_basename.txt
perl -w $REBATE_SCRIPT $FILE $REBATE_LIST_FILE $YYYYMMDD > $REBATE_DATA_FILE ;


if [[ -s /tmp/ORS_pnls_diffs.$YYYYMMDD ]] ; 
then
    SUBJECT="Discrepancy in EOD pnl reconciliation for ALL Accounts - "$YYYYMMDD;

    /bin/mail -s "$SUBJECT" -rrecon_ors_mfg_pnl_ny11@circulumvite.com "nseall@tworoads.co.in" < /tmp/ORS_pnls_diffs.$YYYYMMDD;
else
    SUBJECT="Match in EOD pnl reconciliation - "$YYYYMMDD;

    echo $YYYYMMDD > /tmp/ORS_pnls_diffs.$YYYYMMDD;
    echo "Pnls & volumes match for all instruments & all exchanges & all accounts" >> /tmp/ORS_pnls_diffs.$YYYYMMDD;

    /bin/mail -s "$SUBJECT" -rrecon_ors_mfg_pnl_ny11@circulumvite.com "nseall@tworoads.co.in" < /tmp/ORS_pnls_diffs.$YYYYMMDD;
fi;
