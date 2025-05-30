declare -A server_to_ip_map

server_to_ip_map=( ["IND17"]="10.23.227.82" \
                ["IND16"]="10.23.227.81" \
                ["IND18"]="10.23.227.83")

#server_to_ip_map=( ["IND16"]="10.23.227.81")
#wrapper script, please read main script to see the actual logic 
#this just calls the main reverting and udating script 
#on all cash machines
today=`date +"%Y%m%d"`
prev_tmp=`date -d "yesterday" +"%Y-%m-%d"`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today T`
if [ $is_holiday = "1" ];then
	echo "NSE holiday..., exiting";
	exit
fi
cd /spare/local/files/NSE/PreMarketOpenRatio/

if [ ! -f $today ];then
	echo "" | mailx -s " ${host_name} START RATIO : PREMARKET REPORT NOT FOUND" -r \
	"${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" sanjeev.kumar@tworoads-trading.co.in nseall@tworoads.co.in raghunandan.sharma@tworoads-trading.co.in
	exit
fi

prev_day=`/home/pengine/prod/live_execs/update_date ${today} P A`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE ${prev_day} T`
while [ $is_holiday = "1" ];
do
  prev_day=`/home/pengine/prod/live_execs/update_date ${prev_day} P A`
  is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE ${prev_day} T`
done
date_to_grep=${prev_day:0:4}"-"${prev_day:4:2}"-"${prev_day:6:2};

mail_file='/tmp/stoploss_mail'
todays_products_='/tmp/stoploss_update_products'
>${mail_file}

cat ${today} | awk '{if (($2>=2 || $2 <= -2)&& ($2<=9 && $2 >=-9)){print $1,$2}}' | sed -r 's/-([0-9]+\.[0-9]+)/\1-/g;' | sort -k2n | sed -r 's/([0-9]+\.[0-9]+)-/-\1/g;' | tail -25 >${todays_products_}
echo $date_to_grep
for prod in `egrep "${date_to_grep}" /spare/local/tradeinfo/NSE_Files/EarningsReports/consolidated_earnings.csv | awk '{print $1}'`;
do
	if ! grep -q $prod ${todays_products_};then
		echo $prod
    		grep "${prod}" ${today} >> ${todays_products_}
	fi
done

todaysymbols_count=`cat ${todays_products_} | wc -l`

[ ${todaysymbols_count} -eq 0 ] && exit;

echo -e "<br/><table border="1"><thead><th> SYMBOL </th><th> MOVEMENT </th><tbody>" >> ${mail_file}
while read -r line;
do
  prod=`echo $line | awk '{print $1}'`
  ratio=`echo $line | awk '{print $2}'`
  echo "<tr><td>${prod}</td><td>${ratio}</td></tr>" >> ${mail_file}
done < ${todays_products_} 
echo -e "</tbody></table>" >> ${mail_file}


echo -e "<br/><br/>" >> ${mail_file}
for server in "${!server_to_ip_map[@]}";
do
    rm -rf /tmp/stoploss_update_mail_${server}_${today}
    ssh ${server_to_ip_map[$server]} "/home/pengine/prod/live_scripts/update_start_ratio_stoploss.sh $today"
    if [ $? -ne 0 ];
    then
        echo "<h3>${server} => FAILED TO ADJUST <h3/>" >> ${mail_file}
    else
        scp ${server_to_ip_map[$server]}:/tmp/stoploss_update_mail_${today} /tmp/stoploss_update_mail_${server}_${today}
    fi
done


for server in "${!server_to_ip_map[@]}";
do
	[ ! -f /tmp/stoploss_update_mail_${server}_${today} ] && continue;
	echo -e "<h2>$server</h2>" >> ${mail_file}
	echo -e "<table border="1"><thead><th> SYMBOL </th><th> OLD VALUE </th> <th> NEW VALUE</th>" >> ${mail_file}
	echo -e "<tbody>" >> ${mail_file}
	while read -r line;
	do
	echo $line
		symbol=`echo $line | awk '{print $1}'`
		old_value=`echo $line | awk '{print $2}'`
		new_value=`echo $line | awk '{print $3}'`
		echo -e "<tr><td>${symbol}</td><td>${old_value}</td><td>${new_value}</td></tr>" >> ${mail_file}
	done < /tmp/stoploss_update_mail_${server}_${today}
	echo -e "</tbody></table>" >> ${mail_file}
	echo -e "<br/><br/><br/>" >> ${mail_file}
done

(echo To: "sanjeev.kumar@tworoads-trading.co.in nseall@tworoads.co.in raghunandan.sharma@tworoads-trading.co.in" ; echo From: "raghunandan.sharma@tworoads-trading.co.in"; echo Subject: "[${today}] STOPLOSS ADJUSTMENT"; echo "Content-Type: text/html;";cat /tmp/stoploss_mail) | /usr/sbin/sendmail -t
#(echo To: "raghunandan.sharma@tworoads-trading.co.in " ; echo From: "sanjeev.kumar@tworoads-trading.co.in"; echo Subject: "[${today}] STOPLOSS ADJUSTMENT"; echo "Content-Type: text/html;";cat /tmp/stoploss_mail) | /usr/sbin/sendmail -t

#now sleep for start ratio to stop, and then revert all stop losses
sleep 1080
#sleep 20s
echo "<h2>STOPLOSS REVERT</h2>" >>${mail_file}
for server in "${!server_to_ip_map[@]}";
do
    rm -rf /tmp/stoploss_revert_mail_${server}_${today}
    echo "Revert script called"
    ssh ${server_to_ip_map[$server]} "/home/pengine/prod/live_scripts/revert_start_ratio_stoploss.sh $today"
    echo "Revert Script completed"
    if [ $? -ne 0 ];
    then
        echo "<h3>${server} => FAILED TO REVERT<h3/>" >> ${mail_file}
    else
        scp ${server_to_ip_map[$server]}:/tmp/stoploss_revert_mail_${today} /tmp/stoploss_revert_mail_${server}_${today}
    fi
done



for server in "${!server_to_ip_map[@]}";
do
	[ ! -f /tmp/stoploss_revert_mail_${server}_${today} ] && continue;
	echo -e "<h2>$server</h2>" >> ${mail_file}
	echo -e "<table border="1"><thead><th> SYMBOL </th><th> NEW VALUE</th>" >> ${mail_file}
	echo -e "<tbody>" >> ${mail_file}
	while read -r line;
	do
		symbol=`echo $line | awk '{print $1}'`
		val=`echo $line | awk '{print $2}'`
		echo -e "<tr><td>${symbol}</td><td>${val}</td></tr>" >> ${mail_file}
	done < /tmp/stoploss_revert_mail_${server}_${today}
	echo -e "</tbody></table>" >> ${mail_file}
	echo -e "</br/>" >> ${mail_file}
done

(echo To: "sanjeev.kumar@tworoads-trading.co.in  nseall@tworoads.co.in raghunandan.sharma@tworoads-trading.co.in" ; echo From: "raghunandan.sharma@tworoads-trading.co.in"; echo Subject: "[${today}] STOPLOSS REVERT"; echo "Content-Type: text/html;";cat /tmp/stoploss_mail) | /usr/sbin/sendmail -t
#(echo To: "raghunandan.sharma@tworoads-trading.co.in" ; echo From: "sanjeev.kumar@tworoads-trading.co.in"; echo Subject: "[${today}] STOPLOSS REVERT"; echo "Content-Type: text/html;";cat /tmp/stoploss_mail) | /usr/sbin/sendmail -t

rm -rf ${mail_file} /tmp/stoploss_update_mail_IND16_${today} /tmp/stoploss_update_mail_IND17_${today} /tmp/stoploss_update_mail_IND18_${today}
rm -rf ${mail_file} /tmp/stoploss_revert_mail_IND16_${today} /tmp/stoploss_revert_mail_IND17_${today} /tmp/stoploss_revert_mail_IND18_${today}

