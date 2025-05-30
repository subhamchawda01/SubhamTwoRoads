#!/bin/bash
mail_file=/tmp/run_check_daily_bse_mail_file.html
temp_mail_file=/tmp/check_daily_bse_mail_file

print_usage_and_exit () {
    echo "$0 YYYYMMDD" ;
    exit ;
}

send_mail() {
   (echo "To: raghunandan.sharma@tworoads-trading.co.in, hardik.dhakate@tworoads-trading.co.in, sanjeev.kumar@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, naman.jain@tworoads-trading.co.in, tarun.joshi@tworoads-trading.co.in"
   echo "Subject: BSE DAILY FILES PROBLEM"
   echo "Content-Type: text/html"
   echo
   cat $mail_file
   echo
   ) | /usr/sbin/sendmail -t
}

declare -A server_ip_map
server_ip_map=( ["INDB11"]="192.168.132.11" \
                ["INDB12"]="192.168.132.12")
#server_ip_map=(["IND13"]="10.23.227.63")
#main
if [ $# -ne 1 ] ; then
  echo "Called As : " $* ;
  print_usage_and_exit ;
fi
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $1 T`
if [ $is_holiday = "1" ] ; then
   echo "BSE Holiday. Exiting...";
   exit;
fi
rm -rf $mail_file
for server in "${!server_ip_map[@]}";
do
      ssh ${server_ip_map[$server]} "export LD_LIBRARY_PATH=/apps/anaconda/anaconda3/lib; /home/pengine/prod/live_scripts/check_daily_bse_files_to_file.sh $1  \> /dev/null 2\>\&1"
#      ssh ${server_ip_map[$server]} "/tmp/check_daily_files.sh  \> /dev/null 2\>\&1"
      if ssh ${server_ip_map[$server]} stat $temp_mail_file \> /dev/null 2\>\&1
      then
            scp "dvctrader@"${server_ip_map[$server]}":"$temp_mail_file $temp_mail_file
            echo "File exists Server "$server
            echo "<h2> Machine $server </h2>">>$mail_file
            while read line ;
            do
                     echo "<p>$line</p>" >> $mail_file            
            done < $temp_mail_file
            echo "<hr>" >>$mail_file
            fi
done

[ -f ${mail_file} ] && send_mail
