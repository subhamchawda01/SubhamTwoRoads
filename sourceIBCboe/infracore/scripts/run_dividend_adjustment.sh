declare -A server_to_ip_map

server_to_ip_map=( ["IND17"]="10.23.227.82" \
                ["IND16"]="10.23.227.81" \
                ["IND18"]="10.23.227.83")

today_=`date +"%Y%m%d"`
today_tmp=`date +"%Y-%m-%d"`;

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today_ T`
if [ $is_holiday = "1" ];then
    echo "NSE holiday..., exiting";
    exit
fi

cd /spare/local/tradeinfo/NSE_Files/DividendsReports


>/tmp/dividend_mail_file

echo "=============== SYMBOLS ==============" >/tmp/dividend_mail_file
grep $today_tmp dividends.$today_ | awk 'BEGIN{FS=","}{print $1,$2}' >> /tmp/dividend_mail_file


echo -e "\n" >>/tmp/dividend_mail_file


for server in "${!server_to_ip_map[@]}";
do
    rm -rf /tmp/divideds_mail_file_${server}_${today_}
    echo $server  ${server_to_ip_map[$server]}
    ssh ${server_to_ip_map[$server]} "/home/pengine/prod/live_scripts/dividends_adjustment.sh $today_"
    if [ $? -ne 0 ];
    then
        echo "${server} => FAILED TO ADJUST " >> /tmp/dividend_mail_file
    else
        scp ${server_to_ip_map[$server]}:/tmp/divideds_mail_file_${today_} /tmp/divideds_mail_file_${server}_${today_}
    fi
    ls -lrt /tmp/divideds_mail_file_${server}_${today_}
done

echo -e "\n" >> /tmp/dividend_mail_file


for server in "${!server_to_ip_map[@]}";
do 
    echo "============ $server ================" >>/tmp/dividend_mail_file
    cat /tmp/divideds_mail_file_${server}_${today_} >>/tmp/dividend_mail_file
    echo -e "\n\n" >>/tmp/dividend_mail_file
done


cat /tmp/dividend_mail_file | mailx -s "DIVIDEND ADJUSTMENT" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" nseall@tworoads.co.in raghunandan.sharma@tworoads-trading.co.in
