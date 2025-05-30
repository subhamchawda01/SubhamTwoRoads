#!/bin/bash
download_asmsymbols() {
  #doesn't download the file if the file is same on the server and local
	wget -N --referer https://www1.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www1.nseindia.com/invest/content/Short_Term_ASM.csv 
}

verify_asmsymbols_file() {
	: '
		get start of the day timestamp
		get modification time of downloaded file
	'
	start_timestamp=`date -d $today_ +"%s"`;
	mod_timestamp=`date -r Short_Term_ASM.csv +"%s"`
	if [ $mod_timestamp -lt $start_timestamp  ];then
		echo "" | mailx -s "Failed downloading ASM symbols list " -r "${HOSTNAME}-${USER}<sanjeev.kumar@tworoads-trading.co.in>" sanjeev.kumar@tworoads-trading.co.in nseall@tworoads.co.in
		exit
 	fi
}

: '	
	downloads the file from the website
'
cd /spare/local/tradeinfo/NSE_Files/ASMSecurities/
today_=`date +"%Y%m%d"`;
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today_ T`
if [ $is_holiday = "1" ];then
	echo "NSE holiday..., exiting";
	exit
fi
download_asmsymbols;
verify_asmsymbols_file;
cat Short_Term_ASM.csv | awk 'BEGIN{FS=","}!/SYMBOL/{if ($1 != ""){print $1}}' | egrep -v "\-|selected" | awk '{ gsub (" ", "", $0); print}'  > short_term_asm.csv_$today_

>/tmp/mailfile
#get previous day
previous_working_day=`/home/pengine/prod/live_execs/update_date $today_ P A`;
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $previous_working_day T`
while [ $is_holiday = "1" ];
do
	previous_working_day=`/home/pengine/prod/live_execs/update_date $previous_working_day P A`
	is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $previous_working_day T`
done

diff -u  short_term_asm.csv_$previous_working_day short_term_asm.csv_$today_  | grep -v "\-\-\-" | grep -v "+++"   | grep -v @ > /tmp/products_diff.txt
echo -e "=============== NEW SYMBOLS ==============" >> /tmp/mailfile 
grep "+" /tmp/products_diff.txt | awk '{print substr($1,2);}' >> /tmp/mailfile
echo -e "\n=============  REMOVED SYMBOLS ==========" >> /tmp/mailfile
grep "-" /tmp/products_diff.txt  | awk '{print substr($1,2);}' >> /tmp/mailfile

cat /tmp/mailfile | mailx -s "ASM Symbols - RUN ADDTS ON IND17" -r "${HOSTNAME}-${USER}<sanjeev.kumar@tworoads-trading.co.in>" sanjeev.kumar@tworoads-trading.co.in nseall@tworoads.co.in

#sync to all machines
rsync -ravz --timeout=10 /spare/local/tradeinfo/NSE_Files 10.23.227.82:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=10 /spare/local/tradeinfo/NSE_Files 10.23.227.63:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=10 /spare/local/tradeinfo/NSE_Files 10.23.227.64:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=10 /spare/local/tradeinfo/NSE_Files 10.23.227.65:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=10 /spare/local/tradeinfo/NSE_Files 10.23.227.69:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=10 /spare/local/tradeinfo/NSE_Files 10.23.227.81:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=10 /spare/local/tradeinfo/NSE_Files 10.23.227.82:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=10 /spare/local/tradeinfo/NSE_Files 10.23.227.83:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=10 /spare/local/tradeinfo/NSE_Files 10.23.227.84:/spare/local/tradeinfo --delete-after

chown dvctrader:dvctrader /tmp/mailfile

