#!/bin/bash

GetNearestExpiry() {
  contract_file=${tradeinfo_dir}'/ContractFiles/nse_contracts.'${today}
  expiry=`cat ${contract_file} | grep IDX | grep $shortcode | awk -v date=${today} '{if($6>=date)print $6'} | sort | uniq | head -n1`
}
GetMonthlyNearestExpiry() {
  contract_file=${tradeinfo_dir}'/ContractFiles/nse_contracts.'${today}
  expiry=`cat ${contract_file} | grep IDXFUT | grep BANKNIFTY | awk -v date=${today} '{if($NF>=date)print $NF'} | sort | uniq | head -n1`
}


tradeinfo_dir='/spare/local/tradeinfo/NSE_Files/'
check_shortcode="/home/pengine/prod/live_execs/check_shortcode_valid"
check_path_="/tmp/shortcode_to_process_and_check"
check_valid_shortcode_="/tmp/check_valid_shortcode_123552"
mail_to_send="/tmp/mail_alert_need_to_send_for_the_shortcode_update"
>$mail_to_send

pid_="123552"
date_=`date +%Y%m%d`;
#date_=20220823
next_working_day=`/home/pengine/prod/live_execs/update_date $date_ N A`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
while [[ $is_holiday = "1" ]] 
do
     next_working_day=`/home/pengine/prod/live_execs/update_date $next_working_day N A`
     is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
done
date_=$next_working_day
today=$date_;
shortcode="BANKNIFTY"
GetNearestExpiry
expiry_bnf=$expiry

if [[ ! -f $contract_file ]]
then
  echo "$contract_file File does not Exist"
  echo "$contract_file File does not Exist" >> $mail_to_send
  cat $mail_to_send | mailx -s "Exiting Contract File missing DATE: $date_" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>"  raghunandan.sharma@tworoads-trading.co.in
#, nishit.bhandari@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in
 
	exit
fi
query_file="/spare/local/logs/tradelogs/queryoutput/sms."${date_}.${pid_}
echo "Query File: $query_file"
if [[ $today == $expiry_bnf ]]; then
		LIVE_FILE_=`grep $pid_ /home/dvctrader/ATHENA/run.sh | grep EXPIRY | awk '{print $2}'`
else
		LIVE_FILE_=`grep $pid_ /home/dvctrader/ATHENA/run.sh | grep -v EXPIRY | awk '{print $2}'`
fi
LIVE_FILE_="/home/dvctrader/ATHENA/CONFIG_OPT_IDX_STRANGLE_ALL_RUSH_TMP"
echo "LIVE_FILE: $LIVE_FILE_"
echo "LIVE_FILE:   $LIVE_FILE_" >>$mail_to_send
dir_path_=`echo $LIVE_FILE_ | awk  -F "/" '{print $1"/"$2"/"$3"/"$4"/"$5}'`
bkp_folder_="${dir_path_}_bkp_${date_}"
echo "Dir Path: $dir_path_ 	Bkp Folder: $bkp_folder_"
echo "Dir Path: $dir_path_ 	Bkp Folder: $bkp_folder_" >>$mail_to_send
cd $dir_path_
exit
if [[ $1 == "RECOVER" ]]; then
	echo "Recovering Folder File... $bkp_folder_ to $dir_path_"
	echo "Recovering Folder File... $bkp_folder_ to $dir_path_" >>$mail_to_send
#	rsync -avz  ${bkp_folder_}/ $dir_path_	
# RECOVER Via cmds
	sed -i 's/#HEDGE/HEDGE/' *BANKNIFTY*/Main*
	sed -i 's/#HEDGE/HEDGE/' *BANKNIFTY*/Main* # to remove ##HEDGE to HEDGE
	sed -i 's/STATUS = FALSE/STATUS = TRUE/' *BANKNIFTY*/Main*
	echo "Recovered...   $dir_path_"
	cat $mail_to_send | mailx -s "Recovered Dir for Strat ID: $pid_ DATE: $date_" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>"  raghunandan.sharma@tworoads-trading.co.in
#, nishit.bhandari@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in
	exit;
fi
rsync -avz ${dir_path_}/ $bkp_folder_

pwd
grep 'STATUS = TRUE' NSE_BANKNIFTY_*/Main* | grep _W_ | cut -d'/' -f1  | sed 's/_MM//g' | sed 's/_SQUAREOFF_2//g' | sed 's/_SQUAREOFF//g'  >$check_path_

$check_shortcode $check_path_ $date_ >$check_valid_shortcode_
count_t=`cat $check_valid_shortcode_ | grep Invalid |wc -l`
if [[ $count_t -gt 10 ]]; then
	echo " ShortCode to disable to much... Count $count_t"
	echo " ShortCode to disable to much... Count $count_t"
	cat $check_valid_shortcode_ | grep Invalid | wc -l >> $mail_to_send
	cat $mail_to_send | mailx -s "Strat ID: $pid_ DATE: $date_ Count Greater for Shortcode to disable COUNT: $count_t" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>"  raghunandan.sharma@tworoads-trading.co.in
#, nishit.bhandari@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in
	echo "Exiting..."
	exit
fi
for shortcode in `cat $check_valid_shortcode_ | grep Invalid | awk '{print $1}'`; do
	echo "Disabling $shortcode"
	echo "Disabling $shortcode" >>$mail_to_send
	shortcode_m=`echo $shortcode | sed 's/_W//g'`
	echo "shortcode_Monthly $shortcode_m"
	echo "shortcode_Monthly $shortcode_m" >>$mail_to_send
	sed -i 's/STATUS = TRUE/STATUS = FALSE/' *$shortcode*/Main*
 	hedge_sht=`grep -R $shortcode_m *BANKNIFTY*/Main* | grep HEDGE | grep MM | head -1 | cut -d ':' -f1`
	echo "Hedge_File:   $hedge_sht"
	echo "Hedge_File:   $hedge_sht" >>$mail_to_send
	sed -i 's/HEDGE/#HEDGE/' $hedge_sht
done


cat $mail_to_send | mailx -s "Strat $pid_ Symbol available for $date_" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>"  raghunandan.sharma@tworoads-trading.co.in
#, nishit.bhandari@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in

