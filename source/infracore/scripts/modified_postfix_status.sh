#!/usr/bin/bash

#Check if postfix process is already running


if [[ `ps aux | grep "/usr/libexec/postfix/master" | grep -v 'grep'|wc -c` >  "1" ]]; then #If already running exit
         echo "Postfix Process is running";
else
       echo "Postfix process is not running Restarting "; #If not running

       #Restart
       systemctl stop postfix
       systemctl start postfix
       #Sleep for 10 seconds  
       sleep 1s
       #Check again whether postfix is running or not

       if [[ `ps aux | grep "/usr/libexec/postfix/master" | grep -v 'grep'|wc -c` >  "1" ]]; then
         echo "Postfix Process is running after restart";

        else
               echo "Postfix is not running after restart Sending mail alert";
               ssh dvctrader@10.23.5.67 'echo "" | mailx -s "ERROR::POSTFIX IS NOT RUNNING AFTER RESTART" -r "${HOSTNAME}-${USER}<tarun.joshi@tworoads-trading.co.in>" tarun.joshi@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in; '
        fi
  fi

