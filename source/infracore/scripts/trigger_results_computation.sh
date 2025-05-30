#!/bin/bash
if [ $# -lt 2 ]; then
  echo "$0 LOCATION DATE"
  exit 0
fi

#Location
location=$1
date_=$2
exch_timing_=$3

if [ $date_ = "TODAY" ] ;
then
    date_=$(date "+%Y%m%d")
fi

#Can be any EC2 worker machine
trigger_worker="10.0.1.15"
#In case original worker fails
fallback_worker="10.0.1.77"

failed_products=""
products_to_try=""
result_check_exception_file="/home/pengine/prod/live_configs/result_check_exceptions.cfg"
num_retries=0
sampledata_computed=0
echo "`date` == Script Start ==" >> /spare/local/logs/failedlogs/$location
while [ true ];
do
  if [ "$num_retries" -gt 4 ]; then
    echo "Already tried max possible times. Exiting"
    echo "`date` == Script End ==" >> /spare/local/logs/failedlogs/$location
    /bin/mail -s "Result generation $date_ $location" -r rajlaxmi.sahu@tworoads.co.in rajlaxmi.sahu@tworoads.co.in < /dev/null
    exit 0
  fi
  num_retries=$(($num_retries+1))
  result=`/home/dvcinfra/infracore_install/scripts/check_smart_data_copy_on_file_server.pl $date_ $location $exch_timing_`
  if [ $result -eq "1" ]; then
    echo "`date` Datacopy was successful. Starting results generation $date_ $location"
#Copy was successful, lets start results computation
    worker_load=`ssh dvctrader@$trigger_worker uptime | tr , ' ' | head -1 | awk '{max_load=$11; printf "%d\n",max_load;}'`
    if [ $? -ne 0 ] || [ "$worker_load" == "" ] || [ $worker_load -gt 32 ]; then
      echo "Falling back to $fallback_worker"
      trigger_worker=$fallback_worker
    fi
#TODO: remove this sleep (added for now to comply with time checks in existing rs version)
    if [ "$2" == "TODAY" ] && [ "$num_retries" -le 1 ]; then
      sleep 600
    fi
    echo "`date` == Start ==" >> /spare/local/logs/failedlogs/$location
    if [ "$sampledata_computed" -lt 1 ]; then
        #Sample Data computation
      if [[ "$exch_timing_" > "UTC_1600" ]]; then
        echo "`date` == Computing Sampledata $exch_timing_ =="  >> /spare/local/logs/failedlogs/$location
        `ssh dvctrader@$trigger_worker "/home/dvctrader/basetrade/celeryFiles/celeryClient/celeryScripts/run_my_job.py -m 1 -n hagarwal -f /media/disk-ephemeral2/command_files/sampledata_procs/sd_procs_$location" &>/dev/null`;
        sampledata_computed=1
      else 
          echo "`date` == Not Computing Sampledata $exch_timing_ =="  >> /spare/local/logs/failedlogs/$location
      fi
    fi

    products=`ssh dvctrader@$trigger_worker "/home/dvctrader/basetrade_install/scripts/start_result_generation.sh $location $exch_timing_ $products_to_try" | tr '\n' ' '`
#Sleep for 2 hours, and verify if results were actually computed or not, if not, then notify and start the process all over again
    sleep 7200
    products_to_try=""
    failed_products=""
    for product in $products;
    do
      disabled=`grep -c "$product" $result_check_exception_file`
      if [ "$disabled" -gt 0 ]; then
        #Check has been disabled for this product, continue
        continue
      fi
      percent_computed=`ssh dvctrader@localhost "/home/dvctrader/infracore_install/scripts/check_ec2_globalresults.py $product $date_ | awk '{printf \"%d\n\",\\\$0}'"`
      if [ "${percent_computed:-0}" -lt 90 ]; then
        failed_products="$failed_products $product:$percent_computed"
        products_to_try="$products_to_try,$product"
      fi
    done
    if [ "$failed_products" != "" ]; then
       echo "`date` Result generation $date_ failed using Secondary scheduler for $failed_products ... Trying again in 5 min!" >> /spare/local/logs/failedlogs/$location
      /bin/mail -s "Result generation $date_ failed using Secondary scheduler for $failed_products ... Trying again in 5 min!" -r chandan.kumar@tworoads.co.in chandan.kumar@tworoads.co.in < /dev/null
    else
      echo "`date` == Script End ==" >> /spare/local/logs/failedlogs/$location
      exit 0
    fi
  else
    echo "`date` Retrying results computation"
  fi
#Try again after 10 min
  sleep 600
done
echo "`date` == Script End ==" >> /spare/local/logs/failedlogs/$location
