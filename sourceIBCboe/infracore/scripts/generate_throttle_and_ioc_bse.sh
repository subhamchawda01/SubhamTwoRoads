#!/bin/bash

#End Date
if [ $# -ne 1 ] ; then
  echo "Called As : " $* ;
  echo "$0 DATE[YYYYMMDD]" ;
  exit;
fi


YYYYMMDD=$1
YYYY=${YYYYMMDD:0:4}
MM=${YYYYMMDD:4:2}
DD=${YYYYMMDD:6:2}

echo "Run Mds Log Reader For Date $YYYYMMDD"
# exec is same as mds_log_reader just name change for readability
[ ! -d /NAS1/data/ORSData/ORSBCAST_MULTISHM/BSE/${YYYY}/${MM}/${DD} ] && echo "/NAS1/data/ORSData/ORSBCAST_MULTISHM/BSE/${YYYY}/${MM}/${DD}/ NOT PRESENT IN 10.23.5.62"| mailx -s "ALERT : Failed to Generate BSE Throttle Report for $YYYYMMDD" -r "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in && exit

/home/pengine/prod/live_execs/mds_log_reader_overall_product_throttle "GEN_THROTTLE" "/NAS1/data/ORSData/ORSBCAST_MULTISHM/BSE/${YYYY}/${MM}/${DD}/" ${YYYYMMDD} BSE >/home/dvcinfra/important/ThrottleProject/output_overall_product_throttle_bse 2>&1
echo "Update IOC html"
/home/pengine/prod/live_scripts/ThrottleProject/script/generate_ioc_report_bse.sh
echo "Update Throttle html"
/home/pengine/prod/live_scripts/ThrottleProject/script/generate_throttle_report_bse.sh

