#!/bin/bash
DEST_SERVER="10.23.227.63"

USAGE="$0  YYYYMMDD";

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
remote_Dir="/spare/local/MDSlogs/${yyyy}/${mm}/${dd}/"
local_Dir="/NAS1/data/NSELoggedData/NSE/${yyyy}/${mm}/${dd}/"
mkdir -p $local_Dir

echo "Trying to Sync DATA from ind13"
rsync -avz --progress 10.23.227.63:/spare/local/MDSlogs/${date:0:4}/${date:4:2}/${date:6:2} /NAS1/data/NSELoggedData/NSE/${date:0:4}/${date:4:2}
