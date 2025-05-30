#!/bin/bash 
mail_file=/tmp/bse_mail_user_login
>/tmp/bse_check_user_file.txt;
>$mail_file
declare -i count
YYYYMMDD=`date +\%Y\%m\%d`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
if [ $is_holiday = "1" ] ; then
   echo "BSE Holiday. Exiting...";
   exit;
fi

for srv in 192.168.132.12 192.168.132.11;
do
  echo $srv
  ssh -T $srv < /home/dvcinfra/important/check_procs.sh  >/tmp/bse_check_user_file.txt;
  count=0;
  echo $srv
  while IFS= read -r line; do
     [[ $line == *"==sdv-indb-"* ]] && echo $line  >> $mail_file
     [[ $line == *"DISCONNECT"* ]] && count=0;
     [[ $line == *"Append"* ]] && count=0;
     [[ $line == *"LoggedIn @ :"* ]] && count=$(($count+1));
     [[ $line == *"SOCKET CLOSED ON PEER SIDE @"* ]] && [[ $count -ne 0 ]] && count=$(($count-1));
  done < /tmp/bse_check_user_file.txt
  echo "                           No of User Logged In:                                    $count"
  echo "                           No of User Logged In:                                    $count" >> $mail_file
  addts_count=`ssh -T $srv < /home/dvcinfra/important/check_addts_count.sh | egrep -v 'systemctl|^$'`
  echo "                           Addts Count for Logged In User:                    $addts_count"
  echo "                           Addts Count for Logged In User:                    $addts_count" >> $mail_file
  echo >>$mail_file

done
#echo ""| mailx -s "BSE ORS Login User $YYYYMMDD" -r "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>" subham.chawda@tworoads-trading.co.in < $mail_file
echo ""| mailx -s "BSE ORS Login User $YYYYMMDD" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in < $mail_file

slack_exec="/home/pengine/prod//live_execs/send_slack_notification"
slack_channel="mail-service"
$slack_exec $slack_channel DATA "*${HOSTNAME}-${USER}*\n`cat $mail_file`\n"

#echo ""| mailx -s "ORS Login User $YYYYMMDD" -r "Infra_5.13" subham.chawda@tworoads-trading.co.in < $mail_file
#echo ""| mailx -s "ORS Login User $YYYYMMDD" -r "Infra_5.13" raghunandan.sharma@tworoads-trading.co.in < $mail_file

