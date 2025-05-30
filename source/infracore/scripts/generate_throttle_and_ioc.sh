#!/bin/bash

#End Date
if [ $# -ne 1 ] ; then
  echo "Called As : " $* ;
  echo "$0 DATE[YYYYMMDD]" ;
  exit;
fi


YYYYMMDD=$1
echo "Run Mds Log Reader For Date $YYYYMMDD"
# exec is same as mds_log_reader just name change for readability
/home/pengine/prod/live_execs/mds_log_reader_overall_product_throttle "GEN_THROTTLE" "/NAS1/BACKUP/ORSBCAST_MULTISHM_Q19/" ${YYYYMMDD} >/home/dvcinfra/important/ThrottleProject/output_overall_product_throttle 2>&1
echo "Update IOC html"
/home/pengine/prod/live_scripts/ThrottleProject/script/generate_ioc_report.sh
echo "Update Throttle html"
/home/pengine/prod/live_scripts/ThrottleProject/script/generate_throttle_report.sh

