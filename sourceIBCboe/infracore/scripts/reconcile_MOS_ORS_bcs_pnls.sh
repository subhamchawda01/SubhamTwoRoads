#!/bin/bash

OPEN_POSITION_FILE="/tmp/trades/ALL_OPEN_POSITIONS" ;

FILE="/tmp/trades/ALL_TRADES";
PNL_SCRIPT="/home/dvcinfra/LiveExec/scripts/bcs_ors_pnl_csv.pl";

YYYYMMDD=$1 ;

if [ ! -d /tmp/trades ] ;
then
    mkdir /tmp/trades;
fi

> $FILE;
> /tmp/ORS_pnls_diffs.$YYYYMMDD;

#MICEX
scp -q dvcinfra@172.18.244.107:/spare/local/ORSlogs/MICEX/MICEXPROD01/trades.$YYYYMMDD /tmp/allmicex11.trades.$YYYYMMDD
cat /tmp/allmicex11.trades.$YYYYMMDD >> $FILE;

#RTS
scp -q dvcinfra@172.18.244.107:/spare/local/ORSlogs/RTS/FORTSPROD01/trades.$YYYYMMDD /tmp/allrts11.trades.$YYYYMMDD
cat /tmp/allrts11.trades.$YYYYMMDD >> $FILE;

perl -w $PNL_SCRIPT $FILE $YYYYMMDD > /tmp/ORS_pnls_diffs.$YYYYMMDD;

if [[ -s /tmp/ORS_pnls_diffs.$YYYYMMDD ]] ; 
then
    SUBJECT="ALERT : Discrepancy in MOS pnl reconciliation - "$YYYYMMDD;
    /bin/mail -s "$SUBJECT" "nseall@tworoads.co.in" < /tmp/ORS_pnls_diffs.$YYYYMMDD;
fi
