#!/bin/bash

DIR="/spare/local/ORSlogs/NSE_FO/MSFO"
send_slack_exec=/home/pengine/prod/live_execs/send_slack_notification

date_=`date +%Y%m%d`

export LD_PRELOAD=/home/dvcinfra/important/libcrypto.so.1.1

tail -10 $DIR/log.$date_ | strings | grep -v "\b20" | grep -v "\b9" | grep -i reject >/tmp/ors_rejects_logsi
product=`tail -10 $DIR/log.$date_ | strings | grep -v "\b20" | grep -v "\b9" | grep -i reject | tail -1 | cut -d' ' -f5`
reason=`tail -10 $DIR/log.$date_ | strings | grep -v "\b20" | grep -v "\b9" | grep -i reject | tail -1 | cut -d' ' -f1`
echo "Reject for: "$product
if [[ -s /tmp/ors_rejects_logsi ]];then
   shortcode=`/home/pengine/prod/live_execs/get_shortcode_for_symbol $product $date_`
echo $shortcode
   $send_slack_exec midterm-order-rejects DATA "Reject: $shortcode in $reason"	
fi
