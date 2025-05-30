#!/bin/bash

USAGE1="$0 YYYYMMDD SUMMARY_DAYS_FROM_GIVENDATE" 
EXAMP1="$0 20141015 180"

if [ $# -lt 2 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

YYYYMMDD=$1;
if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
elif [ $YYYYMMDD = "YESTERDAY" ] ;
then
    YYYYMMDD=$(cat /tmp/YESTERDAY_DATE)
fi

SUMMARYDAYS=$2 ; 
CALC=$HOME/infracore/scripts/sumcalc.pl
MESSAGECOUNT_EXEC=$HOME/LiveExec/bin/get_approximate_ors_exchange_message_count ; 
EUREXORSDATALOCATION=/NAS1/data/ORSData/FR2/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2} ; 
MESSAGE_COUNT_DATABASE=/spare/local/files/EurexMessageCountDatabase/message_count_database.db ; 

if [ `ls $EUREXORSDATALOCATION/* | wc -l` -eq 0 ] 
then
 
   echo "ORSDATA NOT PRESENT FOR : " $YYYYMMDD ; 
   exit ; 

fi 

for files in `ls $EUREXORSDATALOCATION/*`; do echo "Product : " $files " EU_COUNT : " `$MESSAGECOUNT_EXEC $files $YYYYMMDD CET_800 EST_800` " US_COUNT : " `$MESSAGECOUNT_EXEC $files $YYYYMMDD EST_800 EST_1600` ; done ; 

printf "\n\n" ;

LASTDAY_COUNT=`for files in \`ls $EUREXORSDATALOCATION/*\`; do $MESSAGECOUNT_EXEC $files $YYYYMMDD ; done | $CALC` ; 

if [ `grep "$YYYYMMDD" $MESSAGE_COUNT_DATABASE | wc -l | awk '{print $1}'` -eq 0 ] 
then

    echo $YYYYMMDD" "$LASTDAY_COUNT >> $MESSAGE_COUNT_DATABASE ; 

fi     

if [ "`tail -n 1 $MESSAGE_COUNT_DATABASE | awk '{print $1}'`" == "$YYYYMMDD" ] 
then
   
   TOTAL_DAYS=`tail -n $SUMMARYDAYS $MESSAGE_COUNT_DATABASE | wc -l` ; 
   TOTAL_COUNT=`tail -n $SUMMARYDAYS $MESSAGE_COUNT_DATABASE | awk '{print $2}' | $CALC` ; 
   AVG=`printf "%d" $((TOTAL_COUNT/TOTAL_DAYS))` ; 

   echo "DATE_EXEUCTED -> " $YYYYMMDD " MESSAGE_COUNT -> " $LASTDAY_COUNT " SUMMARIZED OVER DAYS ( FROM EXECUTED DATE ) -> " $SUMMARYDAYS " AVG_COUNT -> " $AVG ; 

else 

TEMP_FILE=/tmp/message_count.tmp 
> $TEMP_FILE ; 

   for lines in `cat $MESSAGE_COUNT_DATABASE | tr ' ' '~'` 
   do

      yyyymmdd=`echo $lines | awk -F"~" '{print $1}'` ;
      
     if [ "$yyyymmdd" == "$YYYYMMDD" ]  
     then


         echo $lines | tr '~' ' ' >> $TEMP_FILE ; 
         TOTAL_DAYS=`tail -n $SUMMARYDAYS $TEMP_FILE | wc -l`; 
         TOTAL_COUNT=`tail -n $SUMMARYDAYS $TEMP_FILE | awk '{print $2}' | $CALC` ; 
         AVG=`printf "%d" $((TOTAL_COUNT/TOTAL_DAYS))` ;

         echo "DATE_EXEUCTED -> " $YYYYMMDD " MESSAGE_COUNT -> " $LASTDAY_COUNT " SUMMARIZED OVER DAYS ( FROM EXECUTED DATE ) -> " $SUMMARYDAYS " AVG_COUNT -> " $AVG ;  
         exit ;  #Done here 

     else 

        echo $lines | tr '~' ' ' >> $TEMP_FILE ; 

     fi 

   done 

rm -rf $TEMP_FILE ;    

fi 
                                        
