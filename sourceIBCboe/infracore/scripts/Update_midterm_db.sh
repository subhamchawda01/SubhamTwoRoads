#!/bin/bash

YYYYMMDD=$1;
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi

next_working_day=`/home/pengine/prod/live_execs/update_date $YYYYMMDD N W`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
while [ $is_holiday = "1" ]
do
     next_working_day=`/home/pengine/prod/live_execs/update_date $next_working_day N W`
     is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
done
echo  "NextDay: $next_working_day";


#Update midterm_db with Lotsizes for next trading day
cp /spare/local/tradeinfo/NSE_Files/midterm_db /spare/local/tradeinfo/NSE_Files/midterm_db.bkp
cp /spare/local/tradeinfo/NSE_Files/midterm_db /home/dvctrader/trash/midterm_db
/apps/anaconda/anaconda3/bin/python /home/dvctrader/infracore/scripts/Update_MidTermDB_Lotsizes.py $next_working_day /home/dvctrader/trash/midterm_db
mv /home/dvctrader/trash/midterm_db /spare/local/tradeinfo/NSE_Files/midterm_db
chown dvctrader:infra /spare/local/tradeinfo/NSE_Files/midterm_db

