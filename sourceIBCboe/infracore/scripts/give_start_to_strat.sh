#!/bin/bash

today_=`date +"%Y%m%d"`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today_ T`
if [ $is_holiday = "1" ];then
     echo "NSE holiday..., exiting";
     exit
fi

echo "Giving start to IND14"
for i in 123441 123442 123553 123554; do ssh dvctrader@10.23.227.64  "/home/pengine/prod/live_execs/user_msg --traderid $i --start"; done
echo "Giving start to IND15"
for i in 123450 123451 123452 123475; do /home/pengine/prod/live_execs/user_msg --traderid $i --start ; done
echo "Giving start to IND16"
for i in 123706 123707 123708 ; do ssh dvctrader@10.23.227.81 "/home/pengine/prod/live_execs/user_msg --traderid $i --start" ; done

echo "Giving start to IND17"
for i in 123808 123807 123806 123805 123804 ; do ssh dvctrader@10.23.227.82 "/home/pengine/prod/live_execs/user_msg --traderid $i --start" ; done
echo "Giving start to IND18"
for i in 123461 123515 123516 123517 123519 123520; do ssh dvctrader@10.23.227.83 "/home/pengine/prod/live_execs/user_msg --traderid $i --start" ; done

echo "Giving start to IND19"
for i in 123468 123467 123493 123491 123492 ; do ssh dvctrader@10.23.227.69 "/home/pengine/prod/live_execs/user_msg --traderid $i --start"; done
echo "Giving start to IND20"
for i in 123551 123553 123424 123423 123422 ; do ssh dvctrader@10.23.227.84 "/home/pengine/prod/live_execs/user_msg --traderid $i --start" ; done
echo "Giving start to IND23"
for i in 123944 123943 123942 123941 ; do ssh dvctrader@10.23.227.72 "/home/pengine/prod/live_execs/user_msg --traderid $i --start" ; done
echo "Giving Start To ind22"
for i in 123554 123552 123555; do ssh dvctrader@10.23.227.71 "/home/pengine/prod/live_execs/user_msg --traderid $i --start" ; done

echo "" | mailx -s "Start Given to Strats" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in 
