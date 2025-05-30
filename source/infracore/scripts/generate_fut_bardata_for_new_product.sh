#!/bin/bash

#End Date
if [ $# -ne 3 ] ; then
  echo "Called As : " $* ;
  echo "$0 ENDDATE+1[YYYYMMDD] FILE[eg /home/dvcinfra/raghu/GENERATE_OLD_MIDTERMDATA/midterm_prod ] OUTPUTPATH" ;
  exit;
fi

#start Date
date_=20190101
end_date_=$1
FILE_PROD=$2
OUTPUT_PATH=$3
echo "\nNOTE: NEED TO RECOMPILE THE CODE WITH NSELOGGED CPP POINTING TO CORRECT PATH IN LOGGGED DATA DIR\n"
echo "STARTDATE: $date_ ENDDATE: $end_date_ FILE Considered: $FILE_PROD OUTPUTPATH: $OUTPUT_PATH"

while [ $date_ -lt $end_date_ ]; do
  is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date_ T`
  if [ $is_holiday = "1" ] ; then
   echo "$date_ NSE Holiday. Exiting...";
   date_=`date -d "$date_ +1 day" +"%Y%m%d"`
   continue;
  fi
  echo "FOR DATE $date_";
  
  /home//pengine/prod/live_execs/nse_historical_bar_data_generator_new_FO_products $date_ $FILE_PROD IST_915 IST_1530  1 $OUTPUT_PATH
  date_=`date -d "$date_ +1 day" +"%Y%m%d"`
done

cd $OUTPUT_PATH
echo "GENERATED NOW SORTING..."
for pd in `cat $OUTPUT_PATH | awk -F_ '{print $2}' | sort -u`;
do
  echo $pd;
  sort -u $pd | sort -nk1 >temp_prod; 
  mv temp_prod $pd ;
done
