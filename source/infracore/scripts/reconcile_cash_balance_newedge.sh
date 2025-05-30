#!/bin/bash

USAGE1="$0 YYYYMMDD "
EXAMP1="$0 20120301 "

if [ $# -ne 1 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

YYYYMMDD=$1;
PNLFILE=/NAS1/data/MFGlobalTrades/ExchangePnl/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}/pnl.csv ;
MNYFILE=/NAS1/data/MFGlobalTrades/MFGFiles/GMIMNY_$YYYYMMDD".csv" ;

TOTALPNL=`grep -v "BMF" $PNLFILE | tr ',' ' ' | awk '{print $3}' | tr '\n' ' ' | awk '{print $1+$2+$3}' | tr '.' ' ' | awk '{print $1}'`
NTBALCHG=`grep "9Z" $MNYFILE | tail -1 | tr ',' ' ' | awk '{print $10-$9}' | tr '.' ' ' | awk '{print $1}'`

ABSPNL=`echo $TOTALPNL | nawk '{ print ($1 >= 0) ? $1 : 0 - $1}'`
ABSCHG=`echo $NTBALCHG | nawk '{ print ($1 >= 0) ? $1 : 0 - $1}'`

DIFF=`echo $((ABSCHG-ABSPNL)) | nawk '{ print ($1 >= 0) ? $1 : 0 - $1}'`

if [ $DIFF -gt 100 ] ;
then

    echo "CASH BALANCE DISCREPANCY [ " $YYYYMMDD " ] ->  Prev Bal : " `grep "9Z" $MNYFILE | tail -1 | tr ',' ' ' | awk '{print $9}'` " Closing Bal : " `grep "9Z" $MNYFILE | tail -1 | tr ',' ' ' | awk '{print $10}'` " NewEdgePNL : " $TOTALPNL " DIFF : " $DIFF | /bin/mail -s "NewEdge Cash Balance" -r "cashbalance.info@ny11" "ravi.parikh@tworoads.co.in nseall@tworoads.co.in" 

else

    echo "CASH BALANCE MATCH [ " $YYYYMMDD " ] -> Prev Bal : " `grep "9Z" $MNYFILE | tail -1 | tr ',' ' ' | awk '{print $9}'` " Closing Bal : " `grep "9Z" $MNYFILE | tail -1 | tr ',' ' ' | awk '{print $10}'` " NewedgePNL : " $TOTALPNL  " DIFF : " $DIFF | /bin/mail -s "NewEdge Cash Balance" -r "cashbalance.info@ny11" "ravi.parikh@tworoads.co.in nseall@tworoads.co.in" 

fi
