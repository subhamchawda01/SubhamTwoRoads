#!/bin/bash

# Check if gedit is running
# -x flag only match processes whose name (or command line if -f is
# specified) exactly match the pattern. 

date=`date +\%Y\%m\%d`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi

nseRef="/tmp/nse_ref.err"
nseRefMail="/tmp/mail_nse_ref.txt"
lastEditTime=`ls -lrt $nseRef | cut -d' ' -f9`

echo "LA time of the file $lastEditTime" > $nseRefMail
echo "current time ">>$nseRefMail
echo `date` >>$nseRefMail
echo "----Content of the nse_ref error file------------" >>$nseRefMail
tail -50  $nseRef >> $nseRefMail 
echo "-------------------------------------------------" >>$nseRefMail
countFetch=`ps aux | grep fetch_nse_daily_files.sh | wc -l`
if [[ $countFetch > 1 ]]; then 
    if grep -Fq "NSEFILESUPDATED" $nseRefMail; then 
        echo "FETCH NSE IS EXECUTED & RSYNC IS PENDING" >> $nseRefMail
        echo "Running RSYNC " >> $nseRefMail
        /home/dvctrader/important/onexpiry/sync_trade_info.sh >/tmp/sync_again_nse_files 2>&1 &
    else
         echo  "FETCH NSE NOT EXECUTED, Not Running SYNC" >> $nseRefMail
    fi
    echo "" | mailx -s "Fetch_nse is Still Running : ${TIMESTAMP}" raghunandan.sharma@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in < $nseRefMail
else
    /home/dvctrader/important/onexpiry/run_check_daily_files_raghu_mail.sh `date +\%Y\%m\%d`
    if [[ -f "/tmp/run_check_daily_mail_file.html" ]]; then 
               echo "Running syncing as data in not present in all the machines" >> $nseRefMail
               /home/dvctrader/important/onexpiry/sync_trade_info.sh >/tmp/sync_again_nse_files 2>&1 &
    else
               echo "NO RSYNC" >> $nseRefMail
    fi
    echo "" | mailx -s "Fetch_nse is Stopped Running : ${TIMESTAMP}" raghunandan.sharma@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in < $nseRefMail
fi
