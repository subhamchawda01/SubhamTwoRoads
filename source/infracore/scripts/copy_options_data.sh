date=`date +"%Y%m%d"`
if [ $1 == "TODAY" ];
then
  date=`date +"%Y%m%d"`
else
  date=$1
fi

is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date T`
[ $is_holiday_ = "1" ] && exit

i=0
while true; do
    i=$((i+1))
    [[ $i > 8 ]] && break; 
    rsync -avz --progress 10.23.227.63:/spare/local/MDSlogs/${date:0:4}/${date:4:2}/${date:6:2} /NAS1/data/NSELoggedData/NSE/${date:0:4}/${date:4:2}
    status=$?
    [ $status -ne 0 ] && { echo "Retrying..."; sleep 10m; continue;  }
    break;
done

[ $status -ne 0 ] && mailx -s "FAILED OPTIONS DATA SYNC" -r "${HOSTNAME}-${USER}<sanjeev.kumar@tworoads-trading.co.in>" sanjeev.kumar@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in

i=0
while true; do 
     i=$((i+1))
     [[ $i > 8 ]] && break;
     rsync  -avz --progress /NAS1/data/NSELoggedData/NSE/${date:0:4}/${date:4:2}/${date:6:2} dvctrader@3.89.148.73:/NAS1/data/NSELoggedData/NSE/${date:0:4}/${date:4:2}
     [ $? -ne 0 ] && { echo "Retrying..."; sleep 10m; continue;  }
     break;
done
