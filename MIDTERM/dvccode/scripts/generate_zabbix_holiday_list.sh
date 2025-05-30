#!/bin/bash

print_usage_and_exit () {
    echo "$0 YYYYMMDD" ;
    exit ;
}

if [ $# -ne 1 ] ; then
  echo "Called As : " $* ;
  print_usage_and_exit ;
fi

YYYY=$1
previous_year=$((YYYY - 1))
holiday_list_file="/home/pengine/prod/live_configs/exchange_holidays.txt"
cp /home/pengine/prod/live_configs/zabbix_holiday_list_"$previous_year".txt zabbix_holiday_list.txt
if [ -e $holiday_list_file ]
then
  sed -E "s/$previous_year[0-9]*\,*\s*//g" zabbix_holiday_list.txt > new_server_list.txt
  servers=`cat new_server_list.txt | awk '{print $1}'`
  for server in $servers;
  do
    server_holiday_list=`grep -i "$server.*$YYYY" $holiday_list_file | awk '{print $2}' | sed -e :a -e '$!N; s/\n/,/; ta'`;
    echo $server $server_holiday_list;
  done >  server_holiday_list.txt
    join -j 1 new_server_list.txt server_holiday_list.txt > zabbix_holiday_list_"$YYYY".txt
    echo "File " zabbix_holiday_list_"$YYYY".txt " contains Holidays for $YYYY"
else
  echo "Holiday list file " $holiday_list_file " not found"
fi
