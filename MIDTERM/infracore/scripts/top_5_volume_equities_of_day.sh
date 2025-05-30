#!/bin/bash

#fetch & mail top 5 volume equities since morning  

USAGE1="$0 YYYYMMDD "
EXAMP1="$0 YESTERDAY"

if [ $# -ne 1 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

YYYYMMDD=$1

if [ $YYYYMMDD = "YESTERDAY" ] 
then 

    YYYYMMDD=`cat /tmp/YESTERDAY_DATE`

fi

MAIL_FILE=/tmp/top_volume_equities_of_day.txt
EQTY_LIST=/spare/local/files/bmf_equity_list.txt

LOGFILE=/tmp/equity_vol.txt
touch $LOGFILE
>$LOGFILE

touch $MAIL_FILE
>$MAIL_FILE

echo "Date : "$YYYYMMDD >> $MAIL_FILE
echo >> $MAIL_FILE

ALL_VOL_EXEC=$HOME/infracore_install/scripts/all_volumes_of_day.sh 

$ALL_VOL_EXEC BMFEQ $YYYYMMDD > $LOGFILE 2>/dev/null

grep -f $EQTY_LIST $LOGFILE | tr '/' ' ' | tr '_' ' ' | awk '{print $8 "\t" $10 }' | sort -nr -k 2 | head -5 >> $MAIL_FILE

/bin/mail -s "Top 5 Volume Equities" -r "equityvolumesinfo@ny" "nseall@tworoads.co.in" < $MAIL_FILE

rm -rf $LOGFILE
rm -rf $MAILFILE
