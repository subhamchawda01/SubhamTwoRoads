#!/bin/bash

if [ $# -ne 1 ] ;
then
  echo $USAGE
  exit;
fi

if [ $1 == "TODAY" ];
then
   date=`date +"%Y%m%d"`
else
   date=$1
fi

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date T`
if [ $is_holiday = "1" ];then
    echo "NSE holiday..., exiting";
    exit
fi

yyyy=${date:0:4}
mm=${date:4:2}
dd=${date:6:2}


rsync -avz --progress /spare/local/MDSlogs/${date:0:4}/${date:4:2}/${date:6:2}/NSE_[N-Z]* 10.23.5.66:/NAS1/data/NSELoggedData/NSE/${date:0:4}/${date:4:2}/${date:6:2}/
sleep 5m;
rsync -avz --progress /spare/local/MDSlogs/${date:0:4}/${date:4:2}/${date:6:2}/NSE_[N-Z]* 10.23.5.66:/NAS1/data/NSELoggedData/NSE/${date:0:4}/${date:4:2}/${date:6:2}/
sleep 5m;
rsync -avz --progress /spare/local/MDSlogs/${date:0:4}/${date:4:2}/${date:6:2}/NSE_[N-Z]* 10.23.5.66:/NAS1/data/NSELoggedData/NSE/${date:0:4}/${date:4:2}/${date:6:2}/
sleep 5m;
rsync -avz --progress /spare/local/MDSlogs/${date:0:4}/${date:4:2}/${date:6:2}/NSE_[N-Z]* 10.23.5.66:/NAS1/data/NSELoggedData/NSE/${date:0:4}/${date:4:2}/${date:6:2}/
