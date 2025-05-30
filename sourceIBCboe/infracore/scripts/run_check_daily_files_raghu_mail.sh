#!/bin/bash
mail_file=/tmp/run_check_daily_mail_file.html
temp_mail_file=/tmp/check_daily_mail_file

print_usage_and_exit () {
    echo "$0 YYYYMMDD" ;
    exit ;
}

send_mail() {
   (echo "To: raghunandan.sharma@tworoads-trading.co.in, hardik.dhakate@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in"
   echo "Subject: CHECKING FILES ON IND SERVER"
   echo "Content-Type: text/html"
   echo
   cat $mail_file
   echo
   ) | /usr/sbin/sendmail -t
}

declare -A server_ip_map
server_ip_map=( ["IND11"]="10.23.227.61" \
                ["IND12"]="10.23.227.62" \
                ["IND13"]="10.23.227.63" \
                ["IND14"]="10.23.227.64" \
                ["IND15"]="10.23.227.65" \
                ["IND16"]="10.23.227.81" \
                ["IND17"]="10.23.227.82" \
                ["IND18"]="10.23.227.83" \
                ["IND19"]="10.23.227.69" \
                ["IND21"]="10.23.227.66" \
                ["IND22"]="10.23.227.71" \
                ["IND23"]="10.23.227.72" \
                ["IND20"]="10.23.227.84")
#server_ip_map=(["IND13"]="10.23.227.63")
#main
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $1 T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi
if [ $# -ne 1 ] ; then
  echo "Called As : " $* ;
  print_usage_and_exit ;
fi
rm -rf $mail_file
for server in "${!server_ip_map[@]}";
do
      ssh ${server_ip_map[$server]} "/home/pengine/prod/live_scripts/check_daily_files_to_file.sh $1  \> /dev/null 2\>\&1"
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
