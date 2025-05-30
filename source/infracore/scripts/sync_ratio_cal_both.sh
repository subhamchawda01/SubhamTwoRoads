#!/bin/bash

#ssh dvcinfra@10.23.227.66 "/home/pengine/prod/live_scripts/rsync_67_ratio.sh"

echo "RUNING CASH"
#/home/dvcinfra/important/ORS_REPLY_INTRADAILY/get_order_ors.sh >/tmp/ors_ratio_intraday_log.txt 2>&1


#ssh dvcinfra@10.23.227.66 "/home/pengine/prod/live_scripts/rsync_67_fo_ratio.sh"

echo "RUNNING FO"
YYYYMMDD=`date +%Y%m%d` ;
/home/dvcinfra/important/ORS_REPLY_INTRADAILY/script/generate_ors_report_intraday_fo.sh $YYYYMMDD >/tmp/ors_ratio_intraday_log_fo.txt 2>&1
