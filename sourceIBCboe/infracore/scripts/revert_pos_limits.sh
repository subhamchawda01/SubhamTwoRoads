#!/bin/bash
today_=`date +"%Y%m%d"`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today_ T`
if [ $is_holiday = "1" ];then
        echo "NSE holiday..., exiting";
        exit
fi
cd /spare/local/files/NSE/PositionLimits/

cat /home/dvctrader/ATHENA/run.sh | grep -v "^#"|grep -v START_RATIO|grep LIVE | cut -d' ' -f2 >/tmp/live_file_path.txt
>/tmp/live_file_dic.txt

for file in `cat /tmp/live_file_path.txt`;
do
    echo $(dirname "${file}") >>/tmp/live_file_dic.txt
done

sort /tmp/live_file_dic.txt | uniq >/tmp/pos_file_dir.txt
i=0
while read -r filepath;
do
        pos_file="$filepath/PositionLimits.csv";
        limit_file="/spare/local/files/NSE/PositionLimits/limits.${today_}"
        [[ $i -gt 0 ]] && limit_file="/spare/local/files/NSE/PositionLimits/limits.${today_}_${i}"
        i=$((i+1))
        echo "Position File $pos_file"
        echo "Limit File $limit_file"
        touch $limit_file
        while read -r line;
        do
                key=`echo ${line} | awk -F "=" '{print $1}' | awk '{print $1}'`
                value=`echo ${line} | awk -F "=" '{print $2}' | awk '{print $1}'`
                key=${key//&/\\&}
                sed -i "s/$key = .*/$key = $value/g" $pos_file;
        done < $limit_file
done < /tmp/pos_file_dir.txt

pos_file="/home/dvctrader/ATHENA/CONFIG_START_RATIO_20190926/PositionLimits.csv"
limit_file="/spare/local/files/NSE/PositionLimits/limits.${today_}_start"
touch $limit_file
echo "Position File: $pos_file"
echo "Limit File: $limit_file"
while read -r line;
do
      key=`echo ${line} | awk -F "=" '{print $1}' | awk '{print $1}'`
      value=`echo ${line} | awk -F "=" '{print $2}' | awk '{print $1}'`
      key=${key//&/\\&}
      sed -i "s/$key = .*/$key = $value/g" $pos_file;
done < $limit_file

