#!/bin/bash
mail_report="/tmp/eod_of_day_stop.html"
send_mail="/tmp/send_mail_eod_of_day_stop.html"
res=""
true> $mail_report

today=`date +"%Y%m%d"`;	
if [ $# -eq 1 ];
then
    today=$1;
fi

declare -a ids=()
sendMail () {
 
      echo "<h3>Memory : $MEMORY</h3>">>$mail_report
      echo "<h3>Disk : $DISK</h3>">>$mail_report
      echo "<h3>Uptime : $UPTIME</h3>">>$mail_report
   (
#, ravi.parikh@tworoads.co.in, uttkarsh.sarraf@tworoads.co.in, nishit.bhandari@tworoads-trading.co.in
 echo "To: raghunandan.sharma@tworoads-trading.co.in, hardik.dhakate@tworoads-trading.co.in, ravi.parikh@tworoads.co.in, uttkarsh.sarraf@tworoads.co.in, nishit.bhandari@tworoads-trading.co.in"
 echo "Subject: ***************ALERT EOD SCRIPT $res:[ $today]***************"
 echo "Content-Type: text/html"
 echo
 cat "/tmp/eod_of_day_stop.html"
 echo
 ) >$send_mail
 `scp dvctrader@10.23.5.27:$send_mail /tmp/eod_of_day_stop.html`
 `ssh dvctrader@10.23.5.27 "/usr/sbin/sendmail -t /tmp/eod_of_day_stop.html"`
}
init () {
	
	MEMORY=$(free -m | awk 'NR==2{printf "%d", $7*100/$2 }')
	DISK=$(df -k --output=pcent /dev/xvdf | awk '{printf "%d\t", $1 }' | cut -f2)
	UPTIME=$(uptime |cut -d' ' -f14| awk '{printf "%d", $1 }')
	#CPU=$(top -bn1 | grep load | awk '{printf "%.2f%%\t\t\n", $(NF-2)}')
	echo "Memory: "$MEMORY
	echo "Disk: "$DISK
	echo "Uptime: "$UPTIME
	if [ $DISK -gt 95 ] ; then
        	echo "<h3>Script Stopped Due to less Disk Space </h3>">>$mail_report
		res="Stopped"
		sendMail
		exit 0
	elif  [ $MEMORY -lt 40 ] || [ $UPTIME -gt 24 ]
	 then
		ids=($(ps -ef| grep -i trade_engine| cut -d ' ' -f3))
		for id in ${ids[@]} ; do	
			echo "killing process pid : "$id
			echo "<h3>Stopped Process with PID: $id</h3>" >>$mail_report
			`kill $id`
		done
		`sleep 5m`
		MEMORY=$(free -m | awk 'NR==2{printf "%d", $7*100/$2 }')
                UPTIME=$(uptime |cut -d' ' -f14| awk '{printf "%d", $1 }')
		if [ $MEMORY -lt 40 ] || [ $UPTIME -gt 24 ] ; then
		res="Stopped"
		sendMail
	 	exit 0
		fi
		res="Process killed & EOD Script Working"
		sendMail
	fi
	/home/dvctrader/stable_exec/scripts/calc_ratio.sh $today 10 IST_916 IST_930 StartRatio /home/dvctrader/CONFIG_SET/CONFIG_FUT1_RATIO_CALCULATOR/LIVE_FILE.csv 1>/spare/local/logs/log_calc_ratio_start_fut1  2>/spare/local/logs/log_calc_ratio_start_fut1 &

	/home/dvctrader/stable_exec/scripts/calc_ratio.sh $today 11 IST_1500 IST_1525 EndRatio /home/dvctrader/CONFIG_SET/CONFIG_FUT1_RATIO_CALCULATOR/LIVE_FILE.csv 1>/spare/local/logs/log_calc_ratio_end_fut1 2>/spare/local/logs/log_calc_ratio_end_fut1 &

	/home/dvctrader/stable_exec/scripts/calc_ratio.sh $today 12 IST_916 IST_930 StartRatio /home/dvctrader/CONFIG_SET/CONFIG_FUT2_RATIO_CALCULATOR/LIVE_FILE.csv 1>/home/dvctrader/usarraf/log_calc_ratio_start_fut2 2>/home/dvctrader/usarraf/log_calc_ratio_start_fut2 &

	/home/dvctrader/stable_exec/scripts/calc_ratio.sh $today 13 IST_1500 IST_1525 EndRatio /home/dvctrader/CONFIG_SET/CONFIG_FUT2_RATIO_CALCULATOR/LIVE_FILE.csv 1>/home/dvctrader/usarraf/log_calc_ratio_end_fut2 2>/home/dvctrader/usarraf/log_calc_ratio_end_fut2 &

	home/dvctrader/stable_exec/scripts/calc_ratio.sh $today 14 IST_916 IST_930 StartRatio /home/dvctrader/CONFIG_SET/CONFIG_NSE_CM_RATIO_CALCULATOR/LIVE_FILE.csv 1>/spare/local/logs/log_calc_ratio_start_cm1  2>/spare/local/logs/log_calc_ratio_start_cm1 &

	/home/dvctrader/stable_exec/scripts/calc_ratio.sh $today 15 IST_1500 IST_1525 EndRatio /home/dvctrader/CONFIG_SET/CONFIG_NSE_CM_RATIO_CALCULATOR/LIVE_FILE.csv 1>/spare/local/logs/log_calc_ratio_end_cm1 2>/spare/local/logs/log_calc_ratio_end_cm1 &

	 /home/dvctrader/stable_exec/scripts/calc_ratio.sh $today 16 IST_916 IST_930 StartRatio /home/dvctrader/CONFIG_SET/CONFIG_NSE_CM_RATIO_CALCULATOR_FUT1/LIVE_FILE.csv 1>/home/dvctrader/usarraf/log_calc_ratio_start_cm2 2>/home/dvctrader/usarraf/log_calc_ratio_start_cm2 &

	 /home/dvctrader/stable_exec/scripts/calc_ratio.sh $today 17 IST_1500 IST_1525 EndRatio /home/dvctrader/CONFIG_SET/CONFIG_NSE_CM_RATIO_CALCULATOR_FUT1/LIVE_FILE.csv 1>/home/dvctrader/usarraf/log_calc_ratio_end_cm2 2>/home/dvctrader/usarraf/log_calc_ratio_end_cm2 &

	 /home/dvctrader/stable_exec/scripts/generate_bar_data.sh $today ~/RESULTS_FRAMEWORK/strats/prod_file_extended 20 NON 1>/spare/local/logs/log_momentum_adj  2>/spare/local/logs/log_momentum_adj
	
	

}

init $*        
