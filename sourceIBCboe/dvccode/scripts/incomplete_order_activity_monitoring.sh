#!/bin/bash
print_usage_and_exit() {
 echo "$0 YYYYMMDD" ;
 exit ;
}
#Init
ors_exch_trading_locs_file='/home/pengine/prod/live_configs/ors_exch_trading_locs.txt'
ors_message_anaylser="/home/pengine/prod/live_execs/ors_messages_analyser"
ors_base_dir="/NAS1/data/ORSData/"
send_slack_exec=/home/pengine/prod/live_execs/send_slack_notification

if [ $# -ne 1 ] ; then
  echo "Called as : " $* ;
  print_usage_and_exit ;
fi
YYYYMMDD=$1

cat $ors_exch_trading_locs_file | while read location
do
  DD=${YYYYMMDD:6:2}
  MM=${YYYYMMDD:4:2}
  YYYY=${YYYYMMDD:0:4}
  tmp_output_file="/tmp/$location$YYYY$MM$DD""_output"
  tmp_error_file="/tmp/$location$YYYY$MM$DD""_error"
  for ors_file in `ls $ors_base_dir$location/$YYYY/$MM/$DD/ ` ; 
  do
    exch_symbol=`echo $ors_file | awk 'BEGIN{FS=OFS="_"}{NF--; print}' `
    shortcode=`/home/pengine/prod/live_execs/get_shortcode_for_symbol $exch_symbol $YYYYMMDD`
    ors_file_path="$ors_base_dir$location/$YYYY/$MM/$DD/$ors_file"
    $ors_message_anaylser $ors_file_path $shortcode $YYYYMMDD 1> $tmp_output_file 2> $tmp_error_file
    if [ $shortcode ]
    then
      if [ -e "$tmp_error_file" ]
      then
        incomplete_order_count=`cat $tmp_error_file | wc -l`
        if [ "$incomplete_order_count" -ge "1" ]
        then
          error_msg="$incomplete_order_count incomplete orders found for $location on  $YYYYMMDD, File :- $ors_file_path"
          $send_slack_exec sim-infra DATA "$error_msg"
        fi
        rm $tmp_output_file
        rm $tmp_error_file
      fi
    else
      error_msg="Error $ors_file_path"
      $send_slack_exec sim-infra DATA "$error_msg"
    fi
  done
done
