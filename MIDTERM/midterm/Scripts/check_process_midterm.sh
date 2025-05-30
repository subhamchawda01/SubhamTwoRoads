#!/bin/bash
mailfile="/tmp/process_running_file_mailx"

date_=`date +"%Y%m%d"`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date_ T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi

> $mailfile

check_running_process_ind11()
{
    psc=`ssh 10.23.115.61 "ps aux | grep pengine | grep $1 | grep -v grep | wc -l"`

    if [[ $psc -eq $2 ]]; then
        echo "$1 : Running at Ind11:              $psc" >> $mailfile
    else
        echo "$1 : DOWN at Ind11:              $psc" >> $mailfile
    fi
}

check_running_process_ind12()
{
    psc=`ssh 10.23.115.62 "ps aux| grep pengine | grep $1 | grep -v grep | wc -l"`

    if [[ $psc -eq $2 ]]; then
        echo "$1 : Running at Ind12:               $psc" >> $mailfile
    else
        echo "$1 : DOWN at Ind12:             $psc" >> $mailfile
    fi
}

echo "IND11 process"
echo "IND11" >> $mailfile
check_running_process_ind11 notional 3
check_running_process_ind11 mid_term_data_server 1
check_running_process_ind11 CombinedShmWriter 1
check_running_process_ind11 smart_ors_data_logger 1

echo "IND12 process"
echo -e "\nIND12 process Stats " >> $mailfile
check_running_process_ind12 CombinedShmWriter 1
check_running_process_ind12 mid_term_data_server 1

echo "Chanel Details "
channel_info=`ssh 10.23.227.61 "grep STARTED /spare/local/MDSlogs/combined_writer_dbg_${date_}.log |wc -l"`

channel_info12=`ssh 10.23.227.62 "grep STARTED /spare/local/MDSlogs/combined_writer_dbg_${date_}.log |wc -l"`

echo -e "\nChannel receiving data IND11    -->         $channel_info" >> $mailfile

echo "Channel receiving data IND12    -->         $channel_info12" >> $mailfile


echo "Mailx"
[ -s $mailfile ] && cat $mailfile | mail -s "Midterm Process Details  $date_" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in smit@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in  subham.chawda@tworoads-trading.co.in 


