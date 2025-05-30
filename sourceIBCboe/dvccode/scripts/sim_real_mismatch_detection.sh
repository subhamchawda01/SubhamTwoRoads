#!/bin/bash
print_usage_and_exit() {
 echo "$0 YYYYMMDD" ;
 exit ;
}
#Init
exch_trading_locs_file='/home/pengine/prod/live_configs/sim_real_exchtrading_locs.txt'
sim_real_mismatch_detector="/home/pengine/prod/live_execs/sim_real_packet_order_mismatch_detector"
send_slack_exec=/home/pengine/prod/live_execs/send_slack_notification

if [ $# -ne 1 ] ; then
  echo "Called as : " $* ;
  print_usage_and_exit ;
fi
YYYYMMDD=$1

cat $exch_trading_locs_file | while read location
do
  DD=${YYYYMMDD:6:2}
  MM=${YYYYMMDD:6:2}
  YYYY=${YYYYMMDD:0:4}
  tmp_output_file="/tmp/$location$YYYY$MM$DD"
  $sim_real_mismatch_detector $YYYYMMDD $location >> $tmp_output_file  2>&1
  if [ -e "$tmp_output_file" ]
  then
    mismatch_count=`cat $tmp_output_file | grep mismatch.*count | wc -l`
    if [ "$mismatch_count" -ge "1" ]
    then
      error_msg="Packet order Mismatch for $location on  $YYYYMMDD"
      $send_slack_exec sim-infra DATA "$error_msg"
    fi
    rm $tmp_output_file
  fi
done

