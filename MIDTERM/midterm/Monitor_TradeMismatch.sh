#!/bin/bash

BIG_FILE="/home/dvctrader/trash/trademistach_big_all_file"
MISMATCH_FILE="/home/dvctrader/trash/trademistach_last_file"
file="/home/dvctrader/trash/email_alert_trademismatch"

YYYYMMDD=`date +%Y%m%d`;
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
if [ $is_holiday = "1" ] ; then
       echo "NSE Holiday. Exiting...";
       exit;
fi


tail -n300 /spare/local/files/NSE/MidTermLogs/ExecutionTradesMatching | sort | egrep -e "_FUT|_CE_|_PE_" > $BIG_FILE

tail -n10 /spare/local/files/NSE/MidTermLogs/ExecutionTradesMatching | sort | uniq | egrep -e "_FUT|_CE_|_PE_" > $MISMATCH_FILE



>$file
echo "CHECKING SHORTCODE "

while read line; do 
    shortcode=`echo "$line" | awk  -F  "|" '{print $2}' | awk '{print $1}'`
    count_l=`grep $shortcode $BIG_FILE | wc -l`
    echo $line" -> COUNT: "$count_l 
    if [[ $count_l -gt 10 ]];
    then
        echo $line >>$file
    fi
done < $MISMATCH_FILE

echo "Sending Mail "
if [ -s "$file" ]
then
      echo "Mismatch exist"
      cat $file | mailx -s "IND12 TradeMismatch $date" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in  smit@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in;
fi
                                
