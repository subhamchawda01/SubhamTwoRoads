get_expiry_date () {
  ONE=01;

  for i in {1..7}; do 
  	# Iterate on days of last week & check if expiry falls on the day
 	dateStr=`date -d "$YYYY$MM$ONE + 1 month - $i day" +"%w %Y%m%d"`; 
 	# Expiry falls on fourth day of the last week
 	# Check if the day is fourth day that is Thursday
 	if [ ${dateStr:0:1} -eq 4 ] ; then 
 		EXPIRY=${dateStr:2} 
 	fi 
  done
}

print_usage_and_exit () {
    echo "$0 YYYYMMDD" ;
    exit ;
}

if [ $# -ne 1 ] ; then
  echo "Called As : " $* ;
  print_usage_and_exit ;
fi

YYYYMMDD=$1 ;
DD=${YYYYMMDD:6:2}
MM=${YYYYMMDD:4:2}
YY=${YYYYMMDD:2:2}
YYYY=${YYYYMMDD:0:4}

get_expiry_date;

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $EXPIRY T`

# If computed expiry date is a holiday, the previous working day becomes expiry day
if [ $is_holiday != "2" ] ; then
	# Iterate until a working day is found
	for i in {1..7}; do
		ActualExpiry=`date -d  "$EXPIRY - $i day" +"%Y%m%d"`
		is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $ActualExpiry T`
		if [ $is_holiday = "2" ] ; then
			break
		fi
	done
else
# If computed expiry date is not a holiday, then it is expiry date
	ActualExpiry=$EXPIRY
fi

# Send a download notification in case the provided date is expiry date
if [ $YYYYMMDD -eq $ActualExpiry ] ; then
	echo "Download NSE SOS file for the next month " > /spare/local/logs/alllogs/sos_download_alert
else
# No download alert if provided date is not expiry date
	if [ -e /spare/local/logs/alllogs/sos_download_alert ] ; then
		rm /spare/local/logs/alllogs/sos_download_alert
	fi
fi
