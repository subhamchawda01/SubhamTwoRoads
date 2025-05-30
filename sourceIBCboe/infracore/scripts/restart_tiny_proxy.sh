kill -9 `ps aux | grep tinyproxy | grep -v grep | awk '{print $2}'`
/usr/sbin/tinyproxy &
[ $? -ne 0 ] && mailx -s "FAILED STARTING TINY PROXY" -r "${HOSTNAME}-${USER}<sanjeev.kumar@tworoads-trading.co.in>" sanjeev.kumar@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in

