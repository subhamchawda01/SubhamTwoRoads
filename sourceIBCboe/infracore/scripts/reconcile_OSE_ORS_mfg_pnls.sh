#!/bin/bash

OPEN_POSITION_FILE="/tmp/trades/ALL_OPEN_POSITIONS" ;

FILE="/tmp/trades/ALL_TRADES";
PNL_SCRIPT="/home/dvcinfra/LiveExec/scripts/mfg_ors_ose_pnl_csv.pl";

YYYYMMDD=$1 ;

if [ ! -d /tmp/trades ] ;
then
    mkdir /tmp/trades;
fi

> $FILE;

## OSE START ##
OSEDATE=$YYYYMMDD ;
scp -q dvcinfra@10.134.210.184:/spare/local/ORSlogs/OSE/DVC22563/trades.$OSEDATE /tmp/trades
cat /tmp/trades/trades.$OSEDATE >> $FILE; 
## OSE END ##

perl -w $PNL_SCRIPT $FILE $YYYYMMDD > /tmp/ORS_pnls_diffs.$YYYYMMDD;

if [[ -s /tmp/ORS_pnls_diffs.$YYYYMMDD ]] ; 
then
    SUBJECT="ALERT : Discrepancy in OSE pnl reconciliation - "$YYYYMMDD;

    /bin/mail -s "$SUBJECT" -rrecon_ose_ors_pnl_ny11@circulumvite.com "ravi.parikh@tworoads.co.in nseall@tworoads.co.in" < /tmp/ORS_pnls_diffs.$YYYYMMDD;

else
    SUBJECT="Match in OSE EOD pnl reconciliation - "$YYYYMMDD;

    echo $YYYYMMDD > /tmp/ORS_pnls_diffs.$YYYYMMDD;
    echo "Pnls & volumes match for all instruments & all exchanges" >> /tmp/ORS_pnls_diffs.$YYYYMMDD;

    /bin/mail -s "$SUBJECT" -rrecon_ose_ors_pnl_ny11@circulumvite.com "nseall@tworoads.co.in" < /tmp/ORS_pnls_diffs.$YYYYMMDD;
fi;
