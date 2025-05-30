#!/bin/bash
today_=$1
mail_file=/tmp/divideds_mail_file_${today_}
today_tmp=`date +"%Y-%m-%d"`;
>${mail_file}


cp ~/ATHENA/CONFIG_START_RATIO_20190926/PositionLimits.csv_bkp_20200114 ~/ATHENA/CONFIG_START_RATIO_20190926/PositionLimits.csv

cd /spare/local/tradeinfo/NSE_Files/DividendsReports

if [ ! -f dividends.$today_ ];
then	
	exit -1;
fi

for prod in `grep $today_tmp dividends.$today_ | awk 'BEGIN{FS=","}{print $1}'`;
do
	prod=${prod//&/\\&}
	prodname=`echo NSE_$prod"_MAXLONGPOS"`;
	sed -i "s/$prodname = .*/$prodname = 0/g" ~/ATHENA/CONFIG_START_RATIO_20190926/PositionLimits.csv ;
	prodname=`echo NSE_$prod"_MAXSHORTPOS"`;
	sed -i "s/$prodname = .*/$prodname = 0/g" ~/ATHENA/CONFIG_START_RATIO_20190926/PositionLimits.csv ;
 	prodname=`echo NSE_$prod"_MAXLONGEXPOSURE"`;
	sed -i "s/$prodname = .*/$prodname = 0/g" ~/ATHENA/CONFIG_START_RATIO_20190926/PositionLimits.csv ;
	prodname=`echo NSE_$prod"_MAXSHORTEXPOSURE"`;
	sed -i "s/$prodname = .*/$prodname = 0/g" ~/ATHENA/CONFIG_START_RATIO_20190926/PositionLimits.csv ;
done

for prod in `grep $today_tmp dividends.$today_ | awk 'BEGIN{FS=","}{print $1}'`;do
	grep "NSE_${prod}_" ~/ATHENA/CONFIG_START_RATIO_20190926/PositionLimits.csv >>${mail_file}
done


/home/pengine/prod/live_execs/user_msg --traderid 123706 --setmaxpos 1
