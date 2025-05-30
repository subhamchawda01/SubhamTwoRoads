#!/bin/bash

YYYYMMDD=`date +"%Y%m%d"`

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
if [ $is_holiday = "1" ] ; then
    echo "NSE Holiday. Exiting...";
    exit;
fi

#sshpass -p 'tworoads321$' ssh tworoads@10.23.5.20 "C:\\Users\\tworoads\\Desktop\\Postman_Position_File\\CM\\run_loop.bat" & 

while true; do
  /home/pengine/prod/live_scripts/Get_Notis_File_from_window_server.sh
  HHMM=`date +"%H%M"`
  if [ ${HHMM} -gt 1040 ];
  then
      echo "End of the day Exiting...";
      exit 
  fi
  sleep 4m
done

