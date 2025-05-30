#r/bin/bash
if [ $# -ne 1 ];then
	date=`date +"%Y%m%d"`;
else
	date=$1;
fi

nse_audit_parse_exec=/home/pengine/prod/live_execs/nse_audit_in_parse
final_fo_audit=/tmp/final_audit_fo_$date".csv" 
final_cm_audit=/tmp/final_audit_cm_$date".csv"
final_cd_audit=/tmp/final_audit_cd_$date".csv"

for file in `find /reports/Compliance/NSE -type f -name "*$date.in.gz"`;
do
	echo $file
	segment=`echo $file | awk -F"/" '{print $(NF-1)}'`;
	if  [ "$segment" == "MSFO" ] || [ "$segment" == "MSFO3" ] || [ "$segment" == "MEDFO" ] || [ "$segment" == "MSFO4" ] || [ "$segment" == "MSFO5" ] || [ "$segment" == "MSFO6" ] || [ "$segment" == "MSFO7" ] || [ "$segment" == "MSFOIND14" ] || [ "$segment" == "MSFO8" ]; then 
		$nse_audit_parse_exec NSE_FO $file $final_fo_audit;
	elif [ "$segment" == "MSCD" ] ; then 
		$nse_audit_parse_exec NSE_CD $file $final_cd_audit;
	else
		$nse_audit_parse_exec NSE_EQ $file $final_cm_audit;
	fi
done;

cd_buy_turnover=`cat $final_cd_audit | grep -v "TXN_CODE" | egrep "OPTCUR|FUTCUR|FUTIRC" | grep ",B ," | awk -F"," '{print $(NF-1)}' | /home/pengine/prod/live_scripts/sumcalc.pl` ;
fo_buy_turnover=`cat $final_fo_audit | grep -v "TXN_CODE" | egrep -v "FUTCUR|FUTIRC" | grep ",B ," | awk -F"," '{print $(NF-1)}' | /home/pengine/prod/live_scripts/sumcalc.pl` ;
cd_sell_turnover=`cat $final_cd_audit | grep -v "TXN_CODE" | egrep "OPTCUR|FUTCUR|FUTIRC" | grep ",S ," | awk -F"," '{print $(NF-1)}' | /home/pengine/prod/live_scripts/sumcalc.pl` ; 
fo_sell_turnover=`cat $final_fo_audit | grep -v "TXN_CODE" | egrep -v "FUTCUR|FUTIRC" | grep ",S ," | awk -F"," '{print $(NF-1)}' | /home/pengine/prod/live_scripts/sumcalc.pl` ;
cd_net=`echo $cd_buy_turnover" "$cd_sell_turnover | awk '{print $1+$2}'`;
fo_net=`echo $fo_buy_turnover" "$fo_sell_turnover | awk '{print $1+$2}'`;


if [ `wc -l $final_fo_audit | awk '{print $1}'` -gt 1 ] ; then 
  temp_html_file=/tmp/temp_html ;
  >$temp_html_file ;  
  zip audit_file.zip $final_fo_audit

  printf "FO_BUY_TURNOVER -> %.2f\n" $fo_buy_turnover >> $temp_html_file ; 
  printf "FO_SELL_TURNOVER -> %.2f\n" $fo_sell_turnover >> $temp_html_file ; 
  printf "FO_NET_TURNOVER -> %.2f\n" $fo_net >> $temp_html_file ; 
  cat $temp_html_file | mailx -s "NSE_AUDIT FO - $date" -a audit_file.zip -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" "akshay.tk@tworoads-trading.co.in" "nagaraj.aithal@tworoads.co.in" "raghunandan.sharma@tworoads-trading.co.in" "subham.chawda@tworoads-trading.co.in" "infra_alerts@tworoads-trading.co.in"

  rm -rf $temp_html_file audit_file.zip;

  scp $final_fo_audit dvcinfra@10.23.5.67:/reports/Compliance/NSE/AllTradesTxnFiles/ ; 

fi 
if [ -f $final_cd_audit ] && [ `wc -l $final_cd_audit | awk '{print $1}'` -gt 1 ] ; then 
  temp_html_file=/tmp/temp_html ;
  >$temp_html_file ;  

  printf "CD_BUY_TURNOVER -> %.2f\n" $cd_buy_turnover >> $temp_html_file ; 
  printf "CD_SELL_TURNOVER -> %.2f\n" $cd_sell_turnover >> $temp_html_file ; 
  printf "CD_NET_TURNOVER -> %.2f\n\n\n" $cd_net >> $temp_html_file ; 

  cat $temp_html_file | mailx -s "NSE_AUDIT CD - $date" -a $final_cd_audit -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" "akshay.tk@tworoads-trading.co.in" "nagaraj.aithal@tworoads.co.in" "raghunandan.sharma@tworoads-trading.co.in" "subham.chawda@tworoads-trading.co.in" "infra_alerts@tworoads-trading.co.in"
  rm -rf $temp_html_file ;

  scp $final_cd_audit dvcinfra@10.23.5.67:/reports/Compliance/NSE/AllTradesTxnFiles/ ; 

fi 
if [ `wc -l $final_cm_audit | awk '{print $1}'` -gt 1 ] ; then 
  zip audit_file.zip $final_cm_audit
  echo "" | mailx -s "NSE_AUDIT CM - $date" -a audit_file.zip -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" "akshay.tk@tworoads-trading.co.in" "nagaraj.aithal@tworoads.co.in" "raghunandan.sharma@tworoads-trading.co.in" "subham.chawda@tworoads-trading.co.in" "infra_alerts@tworoads-trading.co.in"
  rm -rf $temp_html_file audit_file.zip ;

  scp $final_cm_audit dvcinfra@10.23.5.67:/reports/Compliance/NSE/AllTradesTxnFiles/ ; 

fi 
rm -rf $final_fo_audit $final_cm_audit $final_cd_audit
