#!/bin/bash

if [ $# -lt 4 ]; then
  echo "$0 <fld_path> <event_time> <event_datfile> <shclist>";
  exit 1;
fi

REPO=$HOME"/basetrade";
AfScriptsDir=$REPO"/AflashScripts";

fld_path=$1;
ev_time=$2;
ev_datfile=$3;
shclist=$4;

if [ ! -f $shclist ]; then
  echo "File $shclist NOT found";
  exit 1;
fi

findpxch_script=$AfScriptsDir"/findpxch.sh";

time_prefix=`echo $ev_time | egrep -o '[a-zA-Z]*_'`;
time_numeric=`echo $ev_time | sed 's/[a-zA-Z]*_//g'`;
time_minutes=$(( ((time_numeric/100) * 60) + time_numeric%100 ));
time_min_bef=$(( time_minutes - 20 ));
time_min_aft=$(( time_minutes + 30 ));
time_bef=$time_prefix""$(( ((time_min_bef/60) * 100) + time_min_bef%60 ));
time_aft=$time_prefix""$(( ((time_min_aft/60) * 100) + time_min_aft%60 ));

for shc in `awk '{print $1}' $shclist` ; do mkdir -p $fld_path"/"$shc ; done;

for shc in `awk '{print $1}' $shclist`; do 
  $findpxch_script $shc $ev_datfile $ev_time $time_bef $time_aft $fld_path 2 5 10 20 1>/dev/null 2>&1;
done;

