#!/bin/bash

servers="INDB11 INDB12" ;

declare -A Server_to_Base_Map=([INDB11]="192.168.132.11" [INDB12]="192.168.132.12")

circular_action_file="/tmp/action_report_file"
ipo_addts_mail="/tmp/ipo_addts_mail"
>$ipo_addts_mail

send_mail(){
 cat $ipo_addts_mail
 echo "" | mailx -s "$1" -r "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>" subham.chawda@tworoads-trading.co.in, ravi.parikh@tworoads.co.in, raghunandan.sharma@tworoads-trading.co.in , infra_alerts@tworoads-trading.co.in < $ipo_addts_mail
}

print_msg_and_exit (){
  echo $1;
  exit
}

[ $# -eq 1 ] || print_msg_and_exit "Usage : < script > <ADDTS_0/ADDTS> "

DATE=`date +"%Y%m%d"`
ADDTS_FLAG=$1;
ADJUST_ADDTS_FILE="/tmp/addts_file"
echo "DATE: $DATE"

scp dvcinfra@10.23.5.67:$circular_action_file /tmp/

if [ `grep $DATE $circular_action_file | grep "Listing of Equity Shares" | wc -l` -ne 0 ]; then
  for server in $servers;
  do
     echo "$server"
     echo "$server" >> $ipo_addts_mail

     for product in `grep $DATE $circular_action_file | grep "Listing of Equity Shares" | cut -d' ' -f7` 
     do
       echo "$product"

       if [ "$ADDTS_FLAG" == "ADDTS_0" ]; then

         echo "ssh dvcinfra@${Server_to_Base_Map[$server]} /home/pengine/prod/live_scripts/bse_orsRejectHandling.sh addts BSE_${product} 0"
         ssh dvcinfra@${Server_to_Base_Map[$server]} "/home/pengine/prod/live_scripts/bse_orsRejectHandling.sh addts BSE_${product} 0"
         echo "BSE_${product} 0" >> $ipo_addts_mail

       elif [ "$ADDTS_FLAG" == "ADDTS" ]; then
         
         addts_value=`ssh dvctrader@${Server_to_Base_Map[$server]} "grep -w BSE_${product} $ADJUST_ADDTS_FILE | cut -d' ' -f5"`
         echo "ssh dvcinfra@${Server_to_Base_Map[$server]} /home/pengine/prod/live_scripts/bse_orsRejectHandling.sh addts BSE_${product} $addts_value"
         ssh dvcinfra@${Server_to_Base_Map[$server]} "/home/pengine/prod/live_scripts/bse_orsRejectHandling.sh addts BSE_${product} $addts_value"
         echo "BSE_${product} $addts_value" >> $ipo_addts_mail

       else
         print_msg_and_exit "Usage : < script > <ADDTS_0/ADDTS> "
       fi
       
     done
  done

  send_mail "BSE IPO ADDTS STATUS $DATE" 

fi
