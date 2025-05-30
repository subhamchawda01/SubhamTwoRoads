#r/bin/bash
if [ $# -ne 1 ];then
	date=`date +"%Y%m%d"`;
else
	date=$1;
fi

declare -A server_to_hostname
server_to_hostname=( ["INDB11"]="INDB11" \
                     ["INDB12"]="INDB12" )

bse_audit_parse_exec=/home/pengine/prod/live_execs/bse_audit_in_parse
final_cm_audit=/tmp/bse_final_audit_cm_$date".csv"

for server in "${!server_to_hostname[@]}";
do
  for file in `find /run/media/root/Elements/SERVERDATA/$server/ORSlogs -type f -name "*$date.in.gz"`;
  do
	echo $file
	segment=`echo $file | awk -F"/" '{print $(NF-1)}'`;
	if  [ "$segment" == "MSEQ9" ] ; then 
		$bse_audit_parse_exec BSE_EQ $file $final_cm_audit;
	fi
  done;
done;

if [ `wc -l $final_cm_audit | awk '{print $1}'` -gt 1 ] ; then 
  zip audit_file.zip $final_cm_audit
  echo "" | mailx -s "BSE_AUDIT CM - $date" -a audit_file.zip -r "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>" "akshay.tk@tworoads-trading.co.in" "nagaraj.aithal@tworoads.co.in" "raghunandan.sharma@tworoads-trading.co.in" "subham.chawda@tworoads-trading.co.in" "infra_alerts@tworoads-trading.co.in"
#  echo "" | mailx -s "BSE_AUDIT CM - $date" -a audit_file.zip -r "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>" "subham.chawda@tworoads-trading.co.in" 
  rm -rf audit_file.zip ;

  cp $final_cm_audit /run/media/root/Elements/SERVERDATA/AllTradesTxnFiles/ ; 

fi 
rm -rf $final_cm_audit
