#!/bin/bash
slack_channel='nseinfo'
slack_data_mode='DATA'
slack_exec='/home/pengine/prod/live_execs/send_slack_notification'
action_report_file='/tmp/action_report_file'
action_file="/spare/local/files/NSE/action_to_consider.txt"
temp_file="/tmp/tempory_file"

GetNextWorkingDay(){
    YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD N W`
    is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
    while [ $is_holiday = "1" ] 
    do
        next_working_day=`/home/pengine/prod/live_execs/update_date $YYYYMMDD N W`
        is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
    done
    echo  "NextDay: $YYYYMMDD";
}

SlackNotify(){
    echo "Sending the slack alert"
    `$slack_exec $slack_channel $slack_data_mode "[ $YYYYMMDD ] Actions to be Considered"`
#  while read -r line
#   do 
       $slack_exec $slack_channel FILE ${action_report_file}
#    done < "${action_report_file}"
}

SendMail(){
    echo "Sending mail"
    cut -f2- -d ' ' $action_report_file >$temp_file
    echo "" | mailx -s "REMINDER : Actions to be Considered $YYYYMMDD" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in nseall@tworoads.co.in < $temp_file
}

UpdateFile(){
    echo "updating file"
    grep -v $YYYYMMDD $action_file > $temp_file
    mv $temp_file $action_file
}

HHMM=`date +"%H%M"`
YYYYMMDD=`date +"%Y%m%d"`
if [ ${HHMM} -gt 800 ];
then
  GetNextWorkingDay;
fi
grep $YYYYMMDD $action_file > $action_report_file
#update file in the morning
[ ${HHMM} -lt 800 ] && UpdateFile
#file empty
[[ ! -s ${action_report_file} ]] && exit

if [[ $1 == 'B' ]] || [[ $1 == 'M' ]]; then 
    SendMail
fi
if [[ $1 == 'B' ]] || [[ $1 == 'S' ]]; then
    echo "slackNotify"
    SlackNotify
fi
