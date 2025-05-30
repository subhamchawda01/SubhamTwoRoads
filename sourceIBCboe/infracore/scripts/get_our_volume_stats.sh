#!/bin/bash

USAGE1="$0 YYYYMM [MAIL]"
EXAMP1="$0 ThisMonth [MAIL]"

if [ $# -lt 1 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

YYYYMM=$1
SEND_MAIL="NOMAIL"

if [ $# -gt 1 ] ;
then 

    SEND_MAIL=$2 ;

fi

if [ $YYYYMM = "ThisMonth" ] 
then

    YYYYMM=`date +"%Y%m"` ;

fi


VOL_SCRIPT=$HOME/LiveExec/scripts/get_our_traded_volume_stats_for_shortcode.sh
MAIL_FILE=/tmp/mail_our_volumes.txt

ListOfSymbols="DOL WIN IND WDO DI FBTP FBTS FGBX FOAT FESX FGBS FGBL FGBM FVS ZN ZF ZB ZT UB CGB BAX KFFTI LFZ LFR LFL LFI JFFCE YFEBM NK NIY NKD SXF VX NKM NKMF TOPIX JGBL HHI RI Si GD";

for symbol in $ListOfSymbols
do

    $VOL_SCRIPT $symbol $YYYYMM "SUMMARY" >> $MAIL_FILE

done

if [ $SEND_MAIL = "MAIL" ]
then 
    HOSTNAME=`hostname`;
    /bin/mail -s "OurVolumesThisMonth" -r "ourvolumesinfo@$HOSTNAME" "nseall@tworoads.co.in" < $MAIL_FILE

else

    cat $MAIL_FILE

fi

rm -rf $MAIL_FILE
