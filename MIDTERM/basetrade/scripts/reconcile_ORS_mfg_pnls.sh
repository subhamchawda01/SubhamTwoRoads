#!/bin/bash

FILE="/tmp/trades/ALL_TRADES";
PNL_SCRIPT="/home/dvcinfra/LiveExec/scripts/mfg_ors_pnl_csv.pl";

YYYYMMDD=$(cat /tmp/YESTERDAY_DATE);

if [ ! -d /tmp/trades ] ;
then
    mkdir /tmp/trades;
fi

> $FILE;
scp -q dvcinfra@sdv-chi-srv11:/spare/local/ORSlogs/CME/HC0/trades.$YYYYMMDD $HOME/trades
cat $HOME/trades/trades.$YYYYMMDD > $FILE; > $HOME/trades/trades.$YYYYMMDD;

scp -q dvcinfra@sdv-chi-srv12:/spare/local/ORSlogs/CME/J55/trades.$YYYYMMDD $HOME/trades
cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

scp -q dvcinfra@sdv-chi-srv13:/spare/local/ORSlogs/CME/G52/trades.$YYYYMMDD $HOME/trades
cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

scp -q dvcinfra@sdv-chi-srv14:/spare/local/ORSlogs/CME/VD4/trades.$YYYYMMDD $HOME/trades
cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

scp -q dvcinfra@sdv-fr2-srv11:/spare/local/ORSlogs/EUREX/UTE001/trades.$YYYYMMDD $HOME/trades
cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

scp -q dvcinfra@sdv-fr2-srv11:/spare/local/ORSlogs/EUREX/UTE002/trades.$YYYYMMDD $HOME/trades
cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

scp -q dvcinfra@sdv-fr2-srv12:/spare/local/ORSlogs/EUREX/UTE001/trades.$YYYYMMDD $HOME/trades
cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

scp -q dvcinfra@sdv-fr2-srv12:/spare/local/ORSlogs/EUREX/UTE002/trades.$YYYYMMDD $HOME/trades
cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

scp -q dvcinfra@sdv-fr2-srv13:/spare/local/ORSlogs/EUREX/UTE001/trades.$YYYYMMDD $HOME/trades
cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

scp -q dvcinfra@sdv-fr2-srv13:/spare/local/ORSlogs/EUREX/UTE002/trades.$YYYYMMDD $HOME/trades
cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

scp -q dvcinfra@sdv-fr2-srv14:/spare/local/ORSlogs/EUREX/UTE001/trades.$YYYYMMDD $HOME/trades
cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

scp -q dvcinfra@sdv-fr2-srv14:/spare/local/ORSlogs/EUREX/UTE002/trades.$YYYYMMDD $HOME/trades
cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

scp -q dvcinfra@10.23.182.51:/spare/local/ORSlogs/TMX/BDMA/trades.$YYYYMMDD $HOME/trades
cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

scp -q dvcinfra@10.23.182.52:/spare/local/ORSlogs/TMX/BDMB/trades.$YYYYMMDD $HOME/trades
cat $HOME/trades/trades.$YYYYMMDD >> $FILE; > $HOME/trades/trades.$YYYYMMDD;

> /tmp/ORS_pnls_diffs.$YYYYMMDD;
perl -w $PNL_SCRIPT $FILE $YYYYMMDD > /tmp/ORS_pnls_diffs.$YYYYMMDD;

if [[ -s /tmp/ORS_pnls_diffs.$YYYYMMDD ]] ; 
then
    SUBJECT="Discrepancy in EOD pnl reconciliation - "$YYYYMMDD;

    /bin/mail -s "$SUBJECT" "sghosh@circulumvite.com gchak@circulumvite.com rakesh.kumar@tworoads.co.in" < /tmp/ORS_pnls_diffs.$YYYYMMDD;
#    /bin/mail -s "$SUBJECT" "sghosh@circulumvite.com" < /tmp/ORS_pnls_diffs.$YYYYMMDD;
else
    SUBJECT="Match in EOD pnl reconciliation - "$YYYYMMDD;

    echo $YYYYMMDD > /tmp/ORS_pnls_diffs.$YYYYMMDD;
    echo "Pnls & volumes match for all instruments & all exchanges" >> /tmp/ORS_pnls_diffs.$YYYYMMDD;

    /bin/mail -s "$SUBJECT" "sghosh@circulumvite.com" < /tmp/ORS_pnls_diffs.$YYYYMMDD;
fi;
