#!/bin/bash

#TODO currently unused later on can specify time range etc.
USAGE1="$0 YYYYMMDD"
EXAMP1="$0 20130912"

if [ $# -ne 1 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

YYYYMMDD=$1

if [ "$YYYYMMDD" == "TODAY" ] 
then

    YYYYMMDD=`date +"%Y%m%d"` ;

fi

SHORTCODE="DOL" ;

CAL_DAYS_EXEC=$HOME/infracore_install/bin/calc_prev_day 


export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

MM=${YYYYMMDD:4:2} 

if [ $MM -gt 15 ] 
then 

    PREVYYYYMMDD=`$CAL_DAYS_EXEC $YYYYMMDD 31` 
    PREVMM=${PREVYYYYMMDD:4:2} ;

else     

    PREVYYYYMMDD=`$CAL_DAYS_EXEC $YYYYMMDD 20` 
    PREVMM=${PREVYYYYMMDD:4:2} ;

fi 

TEMP_VOL_FILE=/tmp/our_traded_volumes_temp.txt
>$TEMP_VOL_FILE

MAIL_FILE=/tmp/dol_volumes_monitor.txt 

TOTALDAYS=0

for dt in $YYYYMMDD $PREVYYYYMMDD 
do 

    VOL_DATA_DIR=/NAS1/data/MFGlobalTrades/ProductPnl/${dt:0:4}/${dt:4:2}
    PNL_FILE=pnl.csv

    for dir in `ls $VOL_DATA_DIR`
    do

	if [ -f $VOL_DATA_DIR/$dir/$PNL_FILE ]
	then

	    LINE_COUNT=`grep $SHORTCODE $VOL_DATA_DIR/$dir/$PNL_FILE | wc -l` ;

	    if [ $LINE_COUNT -gt 0 ]
	    then

		((TOTALDAYS++)) ;
		echo $dt `grep $SHORTCODE $VOL_DATA_DIR/$dir/$PNL_FILE | tr ',' ' ' |awk '{if($4 > 0) {print $4} else {print "0"}}'` >> $TEMP_VOL_FILE ;


	    fi ;
	    
	fi

    done

done 

NeededVol=`echo $TOTALDAYS | awk '{print $1*14000}'` ;
TotalVol=`cat $TEMP_VOL_FILE | awk '{sum += $2; print sum}' | tail -1`
MaxVol=`sort -nr -k 2 $TEMP_VOL_FILE | head -1 | awk '{print $2 " On " $1}'`
MinVol=`sort -nr -k 2 $TEMP_VOL_FILE | tail -1 | awk '{print $2 " On " $1}'`
AvgVol=`echo $TotalVol " " $TOTALDAYS | awk '{print $1/$2}' | awk -F"." '{print $1}'` 

SendAlert=false ;

if [ $AvgVol -le 14000 ] 
then
    
    echo $YYYYMMDD $SHORTCODE " -> " "Total : "$TotalVol "TradingDays : "$TOTALDAYS "Max : "$MaxVol "Min : "$MinVol "Avg : "`echo $TotalVol $TOTALDAYS | awk '{print $1/$2}'` " We Lag by -> : " $((NeededVol-TotalVol)) > $MAIL_FILE ;
    HOSTNAME=`hostname`;
    /bin/mail -s "DOL Volume Monitoring" -r "ourvolumesinfo@$HOSTNAME" "nseall@tworoads.co.in" < $MAIL_FILE

fi 

echo $YYYYMMDD $SHORTCODE " -> " "Total : "$TotalVol "TradingDays : "$TOTALDAYS "Max : "$MaxVol "Min : "$MinVol "Avg : "`echo $TotalVol $TOTALDAYS | awk '{print $1/$2}'` ; 

rm -rf $TEMP_VOL_FILE
rm -rf $BAX_TEMP_VOL_FILE
rm -rf $MAIL_FILE 
