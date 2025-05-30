#!/bin/bash
#Cronned on dvcinfra@ny11. Prints yesterday's date to file /tmp/YESTERDAY_DATE (optimization for fetching last working date)
yyyymmdd=`date +\%Y\%m\%d`;
echo $yyyymmdd > /tmp/YESTERDAY_DATE;
