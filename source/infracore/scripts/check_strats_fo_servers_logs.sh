#!/bin/bash

tradelogs="/spare/local/logs/tradelogs/"
querylogs="/spare/local/logs/tradelogs/queryoutput/"
APPEND_FILE="/tmp/log_after_append_strat_logs"

file_after_last_apend(){
  line_no=`grep -nar "/spare/local/logs/tradelogs/ log" $strat_log | tail -1 | cut -d':' -f1`
  tail -n +$line_no $strat_log > $APPEND_FILE
}

date_=`date +%Y%m%d`
strat_mail_file="/tmp/strat_check_mail_file_${date_}"
>$strat_mail_file
count_should_run=0
for script in `crontab -l | grep -v '^#' | grep run | awk '{if(NF == 6) print}' | awk '{print $NF}'`;
do
  for strat_id in `cat $script | grep -v "#" | grep CONFIG | awk '{print $6}' `; do
	count_should_run=$(( count_should_run + 1 ))
	strat_log="/spare/local/logs/tradelogs/log.${date_}.${strat_id}"
        file_after_last_apend
	added_channel=`grep CreateSockets $APPEND_FILE  | grep CHANNEL | grep -v '55.55' |wc -l`
	rec_channel=`grep STARTED $APPEND_FILE | wc -l`
	fut0_count=`grep FUT0 $APPEND_FILE | wc -l`
	fut1_count=`grep FUT1 $APPEND_FILE | wc -l`
	fut2_count=`grep FUT2 $APPEND_FILE | wc -l`
	echo "$strat_id FUT0: $fut0_count	FUT1: $fut1_count FUT2: $fut2_count"
	echo "Channels Added: $added_channel	Getting Data: $rec_channel "
	echo "$strat_id FUT0: $fut0_count     FUT1: $fut1_count FUT2: $fut2_count" >> $strat_mail_file
        echo "Channels Added: $added_channel  Getting Data: $rec_channel " >> $strat_mail_file

  done
done

strat_running=`ps aux | grep trade_engine_live | grep -v grep | wc -l`
echo "		STRAT RUNNING: $strat_running	In Cron: $count_should_run"
echo "          STRAT RUNNING: $strat_running   In Cron: $count_should_run" >> $strat_mail_file

