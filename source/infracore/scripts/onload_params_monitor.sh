#!/bin/bash

#currenly unused later on can choose to monitor specific locations/exchange only
USAGE1="$0 LOCATION"
EXAMP1="$0 ALL"

if [ $# -ne 1 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

ONLOAD_MONITOR_HIST_LOGS_DIR=/home/dvcinfra/onload_monitor_logs
ALL_HOST_ONLOAD_STATUS_FILE=$ONLOAD_MONITOR_HIST_LOGS_DIR/all_host_onload_status.log
ALL_HOST_ONLOAD_STACKDUMP_FILE=$ONLOAD_MONITOR_HIST_LOGS_DIR/all_host_onload_stackdump.log
TEMP_ONLOAD_STACKDUMP=/tmp/onload_stackdump_temp.txt

MAIL_FILE=/tmp/onload_params_alert_mail.txt

THRESHOLD_UDP_PKTS=99
THRESHOLD_TCP_PKTS=10
THRESHOLD_OS_RECV=200000

host_to_monitor=(

  10.23.196.51
  10.23.196.52
  10.23.196.53
  10.23.196.54
  10.23.200.51
  10.23.200.52
  10.23.200.53
  10.23.200.54
  10.23.182.51 #sdv-tor-srv11
  10.23.182.52 #sdv-tor-srv12  
  10.23.23.11 #sdv-bmf-srv11  
  10.23.23.12 #sdv-bmf-srv12 
  10.220.40.1 #sdv-bmf-srv13

)

touch $MAIL_FILE
>$MAIL_FILE 

for host in "${host_to_monitor[@]}"
do

  ## clear file with each host, send out an alert before checking other hosts
  >$MAIL_FILE  
  
  ssh $host 'onload_stackdump lots' > $TEMP_ONLOAD_STACKDUMP

  grep "udp_recvq_pkts" $TEMP_ONLOAD_STACKDUMP | tr '\n' ' ' | awk '{ if ( $2 > '$THRESHOLD_UDP_PKTS' ) { print "Onload Alert On UDP Recvq Pkts : " $2 } }' > $MAIL_FILE
  grep "tcp_recvq_pkts" $TEMP_ONLOAD_STACKDUMP | tr '\n' ' ' | awk '{ if ( $2 > '$THRESHOLD_TCP_PKTS' ) { print "Onload Alert On TCP Recvq Pkts : " $2 } }' >> $MAIL_FILE
  grep "rcv: os=" $TEMP_ONLOAD_STACKDUMP | awk '{ print $2; }' | awk -F= '{ print $2; }' | awk -F\( '{ if ( $1 > '$THRESHOLD_OS_RECV' ) { print "Alert os_recv > 0 : " $1; } }' >> $MAIL_FILE

  LINE_COUNT=`wc -l $MAIL_FILE | awk '{print $1}'`

  if [ $(($LINE_COUNT)) -gt 0 ] 
  then 

     #mail out & save onload stackdump for debugging 

     cat $TEMP_ONLOAD_STACKDUMP >> $MAIL_FILE
     echo `date +"%s"` $host "ONLOAD STACKDUMP" >> $ALL_HOST_ONLOAD_STACKDUMP_FILE
     cat $MAIL_FILE >> $ALL_HOST_ONLOAD_STACKDUMP_FILE
      
     /bin/mail -s "OnloadAlert@$host" -r "onloadmonitor@ny16" "nseall@tworoads.co.in" < $MAIL_FILE ;
     echo `date +"%s"` $host "ALERT" >> $ALL_HOST_ONLOAD_STATUS_FILE ;

  else 
     echo `date +"%s"` $host "OK" >> $ALL_HOST_ONLOAD_STATUS_FILE ;

  fi

done

rm -rf $MAIL_FILE
rm -rf $TEMP_ONLOAD_STACKDUMP
