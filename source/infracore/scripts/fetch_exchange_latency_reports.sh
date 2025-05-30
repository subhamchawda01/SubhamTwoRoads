#!/bin/bash

fetch_latency_reports() {
        YYYYMMDD=$1;
        prev_working_day=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P W`
        prev_working_day=`date -d $prev_working_day  +"%d%m%Y"`
        echo $prev_working_day
        wget --referer https://www1.nseindia.com/products/ --user-agent="Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" -O /spare/local/tradeinfo/NSE_Files/ExchangeLatencies/FO_Latency_Report"$prev_working_day".csv https://www1.nseindia.com/content/FO_Latency_stats"$prev_working_day".csv
        wget --referer  https://www1.nseindia.com/products/ --user-agent="Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" -O /spare/local/tradeinfo/NSE_Files/ExchangeLatencies/CM_Latency_Report"$prev_working_day".csv https://www1.nseindia.com/content/CM_Latency_stats"$prev_working_day".csv
}

fetch_latency_reports `date "+%Y%m%d"`
