#!/bin/bash
mail_tmp="/tmp/strat_running_check_mail"
>$mail_tmp

crontab -l | grep -v "#" | grep Strategy_Runners | grep START | tr ' ' '\n' | grep Strategy | sort | uniq >/tmp/strats_that_should_run


YYYYMMDD=`date +"%Y%m%d"`
is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`

if [ $is_holiday_ = "1" ];
then
    echo "NSE Holiday, Exiting..."
    exit;
fi

date_=`date +"%Y%m%d"`
echo $date_
for line in `cat /tmp/strats_that_should_run`; do 
    strat=`echo $line | cut -d'/' -f6`
    count=`ps aux | grep Strategy_Runners  |grep $strat | wc -l`
    echo "$strat running Count $count"
    if [ $count -lt  1 ];then
        echo "Strategy_Runner $strat not Running" >> $mail_tmp
    fi
done

if [[ -s $mail_tmp ]];then
    
  echo '' | mailx -s "Strat on ind12 not running $date_" raghunandan.sharma@tworoads-trading.co.in  smit@tworoads-trading.co.in <$mail_tmp

fi
