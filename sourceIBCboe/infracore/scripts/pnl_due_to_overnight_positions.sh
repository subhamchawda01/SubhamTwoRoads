#!/bin/bash

PNL_SCRIPT="/home/dvcinfra/LiveExec/scripts/pnl_due_to_overnight_positions.pl";

TODAY=$(date +%Y%m%d)
YESTERDAY=$(cat /tmp/YESTERDAY_DATE);
FILE1="/tmp/trades/ALL_TRADES".$YESTERDAY;
FILE2="/tmp/trades/ALL_TRADES".$TODAY;

if [ ! -d /tmp/trades ] ;
then
    mkdir /tmp/trades;
fi

> $FILE1;
> $FILE2;

## CME
for i in `seq 1 4`;
do
    scp -q dvcinfra@sdv-chi-srv1$i:/spare/local/ORSlogs/CME/MSCME$i/trades.$YESTERDAY /tmp/trades
    cat /tmp/trades/trades.$YESTERDAY >> $FILE1; > /tmp/trades/trades.$YESTERDAY;
    scp -q dvcinfra@sdv-chi-srv1$i:/spare/local/ORSlogs/CME/MSCME$i/trades.$TODAY /tmp/trades
    cat /tmp/trades/trades.$TODAY >> $FILE2; > /tmp/trades/trades.$TODAY;
done

## EUREX
for i in `seq 1 4`;
do
    scp -q dvcinfra@sdv-fr2-srv1$i:/spare/local/ORSlogs/EUREX/NTAPROD*/trades.$YESTERDAY /tmp/trades
    cat /tmp/trades/trades.$YESTERDAY >> $FILE1; > /tmp/trades/trades.$YESTERDAY;
    scp -q dvcinfra@sdv-fr2-srv1$i:/spare/local/ORSlogs/EUREX/NTAPROD*/trades.$TODAY /tmp/trades
    cat /tmp/trades/trades.$TODAY >> $FILE2; > /tmp/trades/trades.$TODAY;
done

## BMF
#for i in `seq 1 4`;
#do
#    scp -q dvcinfra@10.23.23.1$i:/spare/local/ORSlogs/BMFEP/MS00$i/trades.$YESTERDAY /tmp/trades
#    cat /tmp/trades/trades.$YESTERDAY >> $FILE1; > /tmp/trades/trades.$YESTERDAY;
#    scp -q dvcinfra@10.23.23.1$i:/spare/local/ORSlogs/BMFEP/MS00$i/trades.$TODAY /tmp/trades
#    cat /tmp/trades/trades.$TODAY >> $FILE2; > /tmp/trades/trades.$TODAY;
#done

## LIFFE
for i in `seq 1 3`;
do
    scp -q dvcinfra@10.23.52.5$i:/spare/local/ORSlogs/LIFFE/MSBSL$i/trades.$YESTERDAY /tmp/trades
    cat /tmp/trades/trades.$YESTERDAY >> $FILE1; > /tmp/trades/trades.$YESTERDAY;
    scp -q dvcinfra@10.23.52.5$i:/spare/local/ORSlogs/LIFFE/MSBSL$i/trades.$TODAY /tmp/trades
    cat /tmp/trades/trades.$TODAY >> $FILE2; > /tmp/trades/trades.$TODAY;
done

if [[ -s /tmp/trades/OPEN_POSITIONS ]];
    then
    cp /tmp/trades/OPEN_POSITIONS /tmp/trades/OPEN_POSITIONS.$YESTERDAY;
fi

perl -w $PNL_SCRIPT $FILE1 $FILE2 $TODAY > /tmp/trades/OPEN_POSITIONS.$TODAY;
cp /tmp/trades/OPEN_POSITIONS.$TODAY /tmp/trades/OPEN_POSITIONS;

if [[ -s /tmp/trades/OPEN_POSITIONS ]];
then
    echo "Overnight pnl: ";
    cat /tmp/trades/OPEN_POSITIONS;
    SUBJECT="PNL due to overnight positions - "$YESTERDAY;
    /bin/mail -s "$SUBJECT" -rnseall@tworoads.co.in "nseall@tworoads.co.in" < /tmp/trades/OPEN_POSITIONS;
else
    SUBJECT="PNL due to overnight positions - "$YESTERDAY " - no overnight positions";
    echo "No overnight positions on " $YESTERDAY > /tmp/ORS_pnls_diffs.$TODAY;
    /bin/mail -s "$SUBJECT" -rnseall@tworoads.co.in "nseall@tworoads.co.in" < /tmp/overnight_pnl_diffs.$TODAY;
fi;
