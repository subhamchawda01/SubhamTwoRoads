#!/bin/bash
send_slack_exec=/home/pengine/prod/live_execs/send_slack_notification

tail -n50 /spare/local/files/NSE/MidTermLogs/LivePnL | grep TOTAL | awk '{print $2}' >/tmp/currentpnl_irr_numb

date_=`date +"%Y%m%d"`
time_=`date -d "+ 330 minutes" +'%H:%M'`
HHMM=`date +"%H%M"`

sed -i 's/,//g' /tmp/currentpnl_irr_numb


sed -r "s/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[mGK]//g" /tmp/currentpnl_irr_numb >/tmp/f123
rm /tmp/pnl_exist_value_log
currentpnl=`cat /tmp/f123 | cut -d' ' -f1`

awk '{if ( $1 < -2000000 )print "hie "$1; }' /tmp/f123 >/tmp/pnl_exist_value_log

echo $currentpnl

if [[ -s /tmp/pnl_exist_value_log ]];then
    echo "" | mailx -s "ALERT MIDTERM PNL  $currentpnl $time $date" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in smit@tworoads-trading.co.in

    LD_LIBRARY_PATH=/opt/glibc-2.14/lib $send_slack_exec nseinfo  DATA "ALERT Midterm PNL $currentpnl";
fi

