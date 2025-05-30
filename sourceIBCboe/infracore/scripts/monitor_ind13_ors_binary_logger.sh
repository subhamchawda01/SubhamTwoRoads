count=`ps aux | grep ors_binary_logger_multishm | grep -v grep | wc -l`
echo $count
if [ $count -ne 1 ];then
	echo "" | mailx -s "ORS BINARY LOGGER IS DOWN" -r "${HOSTNAME}-${USER}<sanjeev.kumar@tworoads-trading.co.in>" sanjeev.kumar@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in #nseall@tworoads.co.in
fi
