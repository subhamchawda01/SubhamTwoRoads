#!/bin/bash
YYYYMMDD=`date +"%Y%m%d"`
DDMMYYYY=`date +"%d%m%Y"`
FTP_HOST='ftp.connect2nse.com'
MEMBER_ID='90044'
FO_USER_ID='F90044'
CM_USER_ID='90044'
FO_PASSWD='Jan@2073'
CM_PASSWD='Jan@2073'

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi

FILLRATIOSTATSDIR='/var/www/html/FillRatioReport';
FILLRATIOREPORTSDIR=${FILLRATIOSTATSDIR}/Reports;
cd ${FILLRATIOREPORTSDIR}
#download for fo
ftp -n $FTP_HOST <<SCRIPT
	user ${FO_USER_ID} ${FO_PASSWD}
	cd faoftp
	cd F90044
	cd investigation
	cd dnld
	binary
	get FAO_ORDER_TO_TRADE_RATIO_${DDMMYYYY}_${MEMBER_ID}.csv
SCRIPT
#download for cm
ftp -n $FTP_HOST <<SCRIPT
	user ${CM_USER_ID} ${CM_PASSWD}
	cd 90044
	cd Investigation
	cd Dnld
	binary
	get CM_ORDER_TO_TRADE_RATIO_${DDMMYYYY}_${MEMBER_ID}.csv
SCRIPT
pwd
echo  CM_ORDER_TO_TRADE_RATIO_${DDMMYYYY}_${MEMBER_ID}.csv 
if [ ! -f CM_ORDER_TO_TRADE_RATIO_${DDMMYYYY}_${MEMBER_ID}.csv ] || [ ! -f FAO_ORDER_TO_TRADE_RATIO_${DDMMYYYY}_${MEMBER_ID}.csv ];
then
	echo "" | mailx -s "FAILED DOWNLOADING FILL RATIO FILE" -r "${HOSTNAME}-${USER}<sanjeev.kumar@tworoads-trading.co.in>" sanjeev.kumar@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
	exit;
fi
servers="IND11 IND14 IND15 IND16 IND17 IND18 IND19 IND20"; 
fo_servers="IND11 IND14 IND15 IND19 IND20"
cm_servers="IND16 IND17 IND18"
declare -A server_to_userid_map
server_to_userid_map=( ["IND11"]="34353,FO" \
	["IND14"]="36277 36474,FO" \
	["IND15"]="36298 34110,FO" \
	["IND16"]="43965 43740,CM"	\
	["IND17"]="43706 43605,CM" \
	["IND18"]="43603 43966,CM" \
	["IND19"]="37709 36975,FO" \
	["IND20"]="35693 34459,FO" \
	)

#do cleanup
rm -rf ${FILLRATIOSTATSDIR}/${YYYYMMDD}
grep -v ${YYYYMMDD} ${FILLRATIOSTATSDIR}/fill_ratio_hisorical.txt > /tmp/tmp_fillratio.txt
cp --no-preserve=mode /tmp/tmp_fillratio.txt ${FILLRATIOSTATSDIR}/fill_ratio_hisorical.txt
rm /tmp/tmp_fillratio.txt

for server in $servers;
do
	grep -v ${YYYYMMDD} ${FILLRATIOSTATSDIR}/${server}_fill_ratio_report.txt > /tmp/tmp_fillratio.txt
	cp --no-preserve=mode /tmp/tmp_fillratio.txt ${FILLRATIOSTATSDIR}/${server}_fill_ratio_report.txt
	rm /tmp/tmp_fillratio.txt
done

grep -v ${YYYYMMDD} ${FILLRATIOSTATSDIR}/CASH_fill_ratio_report.txt > /tmp/tmp_fillratio.txt
cp --no-preserve=mode /tmp/tmp_fillratio.txt ${FILLRATIOSTATSDIR}/CASH_fill_ratio_report.txt
rm /tmp/tmp_fillratio.txt

grep -v ${YYYYMMDD} ${FILLRATIOSTATSDIR}/FUT_fill_ratio_report.txt > /tmp/tmp_fillratio.txt
cp --no-preserve=mode /tmp/tmp_fillratio.txt ${FILLRATIOSTATSDIR}/FUT_fill_ratio_report.txt
rm /tmp/tmp_fillratio.txt

mkdir -p ${FILLRATIOSTATSDIR}/${YYYYMMDD}
no_of_orders_cm=0;
no_of_orders_fo=0;
no_of_trades_cm=0;
no_of_trades_fo=0;
line="<tr><td>${YYYYMMDD}</td>";
for server in ${servers};
do
	no_of_users=0;
	sum_of_trades=0;
	sum_of_orders=0;
	fill_ratio=0;
	for userid in `echo ${server_to_userid_map[$server]} | awk -F "," '{print $1}'`;
	do
		data_line=`cat ${FILLRATIOREPORTSDIR}/FAO_ORDER_TO_TRADE_RATIO_${DDMMYYYY}_${MEMBER_ID}.csv \
			${FILLRATIOREPORTSDIR}/CM_ORDER_TO_TRADE_RATIO_${DDMMYYYY}_${MEMBER_ID}.csv \
		| awk -F ","  -v t_userid="$userid" '{gsub(" ","",$2);if($2==t_userid){print $0}}'`;
		no_of_orders=`echo $data_line | awk '{gsub(" ","",$3);print $3}'`;
		no_of_trades=`echo $data_line | awk  '{gsub(" ","",$4);print $4}'`;
		fill_ratio=`echo $data_line | awk  '{gsub(" ","",$5);print$5}'`
		sum_of_orders=$((sum_of_orders+no_of_orders));
		sum_of_trades=$((sum_of_trades+no_of_trades));
		no_of_users=$((no_of_users+1));
		data_to_append="<tr>
		<td>${userid}</td>
		<td>${no_of_trades}</td>
		<td>${no_of_orders}</td>
		<td>${fill_ratio}</td></tr>"
		echo $data_to_append >> ${FILLRATIOSTATSDIR}/${YYYYMMDD}/${server}.users.txt
	done
	fill_ratio_avg=`printf '%.3f\n' "$(echo "$sum_of_orders / $sum_of_trades" | bc -l)"`;
	data_to_append="<tr>
	<td>${YYYYMMDD}</td>
	<td><a href=\"${YYYYMMDD}/${server}.users.html\"><img border=0 src=\"Graph-512.png\" width=40 height=25></a></td>
	<td>${sum_of_trades}</td>
	<td>${sum_of_orders}</td>
	<td>${fill_ratio_avg}</td>
	</tr>" 
	echo $data_to_append >> ${FILLRATIOSTATSDIR}/${server}_fill_ratio_report.txt;
	line="${line}<td>${fill_ratio_avg}</td>"
	segment=`echo ${server_to_userid_map[$server]} | awk -F "," '{print $2}'`;
	if [ "$segment" == "CM" ];
	then
	   no_of_orders_cm=$((no_of_orders_cm+sum_of_orders));
	   no_of_trades_cm=$((no_of_trades_cm+sum_of_trades));
	else
	  no_of_orders_fo=$((no_of_orders_fo+sum_of_orders));
 	  no_of_trades_fo=$((no_of_trades_fo+sum_of_trades));	
	fi 
done
total_orders=$((no_of_orders_cm+no_of_orders_fo));
total_trades=$((no_of_trades_cm+no_of_trades_fo));
echo $total_orders" "$total_trades" "$num_of_orders_fo" "$no_of_orders_cm" "$no_of_trades_fo" "$no_of_trades_cm
fill_ratio_cm=`printf '%.3f\n' "$(echo "$no_of_orders_cm / $no_of_trades_cm" | bc -l)"`
fill_ratio_fo=`printf '%.3f\n' "$(echo "$no_of_orders_fo / $no_of_trades_fo" | bc -l)"`
fill_ratio_overall=`printf '%.3f\n' "$(echo "$total_orders / $total_trades" | bc -l)"`
line="${line}<td>${fill_ratio_cm}</td><td>${fill_ratio_fo}</td><td>${fill_ratio_overall}</td></tr>"
echo $line >> ${FILLRATIOSTATSDIR}/fill_ratio_hisorical.txt

data_to_append="<tr>
	<td>${YYYYMMDD}</td>
	<td><a href=\"${YYYYMMDD}/CASH.users.html\"><img border=0 src=\"Graph-512.png\" width=40 height=25></a></td>
	<td>${no_of_trades_cm}</td>
	<td>${no_of_orders_cm}</td>
	<td>${fill_ratio_cm}</td>
</tr>"

echo ${data_to_append} >> ${FILLRATIOSTATSDIR}/CASH_fill_ratio_report.txt;
 
data_to_append="<tr>
	<td>${YYYYMMDD}</td>
	<td><a href=\"${YYYYMMDD}/FUT.users.html\"><img border=0 src=\"Graph-512.png\" width=40 height=25></a></td>
	<td>${no_of_trades_fo}</td>
	<td>${no_of_orders_fo}</td>
	<td>${fill_ratio_fo}</td>
</tr>"

echo $data_to_append >> ${FILLRATIOSTATSDIR}/FUT_fill_ratio_report.txt;
#generate html pages
cat ${FILLRATIOSTATSDIR}/main_page_header.txt >${FILLRATIOSTATSDIR}/index.html
cat ${FILLRATIOSTATSDIR}/fill_ratio_hisorical.txt >>${FILLRATIOSTATSDIR}/index.html
cat ${FILLRATIOSTATSDIR}/main_page_footer.txt >>${FILLRATIOSTATSDIR}/index.html
for server in $servers;
do
	cat ${FILLRATIOSTATSDIR}/machines_pages_header.txt > ${FILLRATIOSTATSDIR}/${server}.index.html
	cat ${FILLRATIOSTATSDIR}/${server}_fill_ratio_report.txt >> ${FILLRATIOSTATSDIR}/${server}.index.html
	cat ${FILLRATIOSTATSDIR}/machines_pages_footer.txt >> ${FILLRATIOSTATSDIR}/${server}.index.html
done

cat ${FILLRATIOSTATSDIR}/machines_pages_header.txt > ${FILLRATIOSTATSDIR}/CASH.index.html
cat ${FILLRATIOSTATSDIR}/CASH_fill_ratio_report.txt >> ${FILLRATIOSTATSDIR}/CASH.index.html
cat ${FILLRATIOSTATSDIR}/machines_pages_footer.txt >> ${FILLRATIOSTATSDIR}/CASH.index.html
cat ${FILLRATIOSTATSDIR}/machines_pages_header.txt > ${FILLRATIOSTATSDIR}/FUT.index.html
cat ${FILLRATIOSTATSDIR}/FUT_fill_ratio_report.txt >> ${FILLRATIOSTATSDIR}/FUT.index.html
cat ${FILLRATIOSTATSDIR}/machines_pages_footer.txt >> ${FILLRATIOSTATSDIR}/FUT.index.html

cat ${FILLRATIOSTATSDIR}/machine_user_page_header.txt > ${FILLRATIOSTATSDIR}/${YYYYMMDD}/CASH.users.html
cat ${FILLRATIOSTATSDIR}/machine_user_page_header.txt > ${FILLRATIOSTATSDIR}/${YYYYMMDD}/FUT.users.html
for server in $servers;
do
  cat ${FILLRATIOSTATSDIR}/machine_user_page_header.txt > ${FILLRATIOSTATSDIR}/${YYYYMMDD}/${server}.users.html
  cat ${FILLRATIOSTATSDIR}/${YYYYMMDD}/${server}.users.txt >> ${FILLRATIOSTATSDIR}/${YYYYMMDD}/${server}.users.html
  cat ${FILLRATIOSTATSDIR}/machine_user_page_footer.txt >> ${FILLRATIOSTATSDIR}/${YYYYMMDD}/${server}.users.html
  segment=`echo ${server_to_userid_map[$server]} | awk -F "," '{print $2}'`;
  if [ "$segment" == "FO" ];
  then
	cat ${FILLRATIOSTATSDIR}/${YYYYMMDD}/${server}.users.txt >> ${FILLRATIOSTATSDIR}/${YYYYMMDD}/FUT.users.html
  else
	cat ${FILLRATIOSTATSDIR}/${YYYYMMDD}/${server}.users.txt >> ${FILLRATIOSTATSDIR}/${YYYYMMDD}/CASH.users.html	
  fi
done
cat ${FILLRATIOSTATSDIR}/machine_user_page_footer.txt >> ${FILLRATIOSTATSDIR}/${YYYYMMDD}/CASH.users.html
cat ${FILLRATIOSTATSDIR}/machine_user_page_footer.txt >> ${FILLRATIOSTATSDIR}/${YYYYMMDD}/FUT.users.html
: '
#Send Alert only for EOD RUN
if [ "$1" == "EOD" ];
then
	if [ $fill_ratio_cm -gt 50 ] || [ $fill_ratio_fo -gt 50 ];
	then
		echo -e "FILL RATIO CM: ${fill_ratio_cm} \n FILL RATIO FO: ${fill_ratio_fo}" | mailx -s "FILL RATIO ALERT" -r sanjeev.kumar@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in
	fi
fi
'
