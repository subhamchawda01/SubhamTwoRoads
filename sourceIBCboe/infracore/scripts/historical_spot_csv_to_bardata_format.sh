#!/bin/bash

#niftybookfile="/home/dvctrader/rahul/OPTIONS/DATASET/INDEX_DATA/niftyIDX_2014Q1_2021Q4_1Min1_Corrected.csv"
bankniftybookfile="/home/dvctrader/rahul/OPTIONS/DATASET/INDEX_DATA/bankniftyIDX_2016Q1_2022Q1_1Min_Corrected.csv"
tmpfile="/tmp/shortcodebookfile_sadasda"
outpath="/home/dvctrader/naman/"

declare -a files=( $bankniftybookfile )

get_expiry_date () {
  ONE=01;
  YYYY=${tradingDate:0:4}
  MM=${tradingDate:4:2}
  EXPIRY=$tradingDate;
  for i in {1..7}; do dateStr=`date -d "$YYYY$MM$ONE + 1 month - $i day" +"%w %Y%m%d"`; if [ ${dateStr:0:1} -eq 4 ] ; then EXPIRY=${dateStr:2}; fi ; done
  if [[ $tradingDate -gt $EXPIRY ]];then
          for i in {1..7}; do dateStr=`date -d "$YYYY$MM$ONE + 2 month - $i day" +"%w %Y%m%d"`; if [ ${dateStr:0:1} -eq 4 ] ; then EXPIRY=${dateStr:2}; fi ; done
  fi
  is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $EXPIRY T`
  while [ $is_holiday = 1 ]; do
        EXPIRY=`date -d "$EXPIRY - 1 day" +%Y%m%d`
        is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $EXPIRY T`
  done
}

for file in ${files[@]}
do
        >$tmpfile
        tail -n +2 $file >> $tmpfile
        echo "***********changing format of file :: $file******************"; echo;
        is_file_banknifty=`echo "$file" | grep -i "banknifty" | wc -l`
#       echo "is banknifty : $is_file_banknifty"
        if [[ $is_file_banknifty -eq 1 ]]; then
                IDXname="NIFTYBANK_FF_0_0"; FileName="NIFTYBANK";
        else
                IDXname="NIFTY_FF_0_0"; FileName="NIFTY50";
        fi
        >"$outpath/$FileName"
#       echo "**************** name : $IDXname *************************"
        while IFS= read -r line
        do
                tradingDate=`echo $line | awk '{print $1}'`
                tradingDate=`date -d "$tradingDate" +%Y%m%d`
#               echo "$tradingDate"
                if [[ $tradingDate -gt 20210822 ]]; then
                        echo "current bardata head date reached **********************************************";
                        exit
                fi
                startdatetime=`echo "$line" | awk 'BEGIN {FS=OFS=","}{print $1}'`
#               echo "starttime:: $startdatetime"
                starttime=`date -d "$startdatetime" +%s`
#               echo "starttime: $starttime"
                open=`echo "$line" | awk 'BEGIN {FS=OFS=","}{print $2}'`
                high=`echo "$line" | awk 'BEGIN {FS=OFS=","}{print $3}'`
                low=`echo "$line" | awk 'BEGIN {FS=OFS=","}{print $4}'`
                close=`echo "$line" | awk 'BEGIN {FS=OFS=","}{print $5}'`
                volume=0 #`echo "$line" | awk 'BEGIN {FS=OFS=","}{print $6}'`
#               echo " open high low close :: $open $high $low $close "
                get_expiry_date;
#               echo "EXPIRY: $EXPIRY"
                echo -e "$starttime \t $IDXname \t $starttime \t $starttime \t $EXPIRY \t $open \t $close \t $low \t $high \t 0 \t $volume \t 1"         >>"$outpath/$FileName"
#               exit
                done< $tmpfile
done

