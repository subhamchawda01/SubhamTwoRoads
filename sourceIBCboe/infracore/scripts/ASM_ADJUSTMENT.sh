#!/bin/bash
today_=`date +"%Y%m%d"`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today_ T`
if [ $is_holiday = "1" ];then
	echo "NSE holiday..., exiting";
	exit
fi

TMP_TS_FILE='/tmp/asm_addts.cfg'
>$TMP_TS_FILE

cd /spare/local/tradeinfo/NSE_Files/ASMSecurities

if [ ! -f short_term_asm.csv_$today_ ];then
	echo "" | mailx -s "IND17 - ASM FILE NOT FOUND - SET ASM SYMBOLS LIMITS TO ZERO MANUALLY" -r sanjeev.kumar@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in nseall@tworoads.co.in raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in
	exit
fi

for prod in `cat short_term_asm.csv_$today_`;
do
	echo "NSE MSEQ3 ADDTRADINGSYMBOL \"NSE_$prod\" 0 0 0 0" >> $TMP_TS_FILE
done

/home/pengine/prod/live_scripts/ADDTRADINGSYMBOL.sh $TMP_TS_FILE

cat /home/dvctrader/ATHENA/run.sh | grep -v "^#"|grep -v START_RATIO|grep LIVE | cut -d' ' -f2 >/tmp/live_file_path.txt
>/tmp/live_file_dic.txt
for file in `cat /tmp/live_file_path.txt`;
do
    echo $(dirname "${file}") >>/tmp/live_file_dic.txt
done
sort /tmp/live_file_dic.txt | uniq >/tmp/pos_file_dir.txt
i=0
for filepath in `cat /tmp/pos_file_dir.txt`;
  do
      pos_file="${filepath}/PositionLimits.csv"
      limit_file="/spare/local/files/NSE/PositionLimits/limits.${today_}"
      [[ $i -gt 0 ]] && limit_file="/spare/local/files/NSE/PositionLimits/limits.${today_}_${i}"
      echo "Postion file: $pos_file"
      echo "Limit file: $limit_file"
      i=$((i+1))
      touch $limit_file
      for prod in `cat short_term_asm.csv_$today_`;
      do
          echo "Symbol Change:- $prod" 
          if ! grep -q "NSE_${prod}_" $pos_file
          then
              echo "Not in Position file, Not Updating.."
              continue;
          fi
          if ! grep -q "NSE_${prod}_" $limit_file
          then
              echo "Adding to Limit File"
              grep "NSE_${prod}_" $pos_file >> $limit_file
          fi
          prod=${prod//&/\\&}
          prodname=`echo "NSE_${prod}_MAXLONGPOS"`;
          sed -i "s/$prodname = .*/$prodname = 0/g" $pos_file ;
          prodname=`echo "NSE_${prod}_MAXSHORTPOS"`;
          sed -i "s/$prodname = .*/$prodname = 0/g" $pos_file ;
          prodname=`echo "NSE_${prod}_MAXLONGEXPOSURE"`;
          sed -i "s/$prodname = .*/$prodname = 0/g" $pos_file ;
          prodname=`echo "NSE_${prod}_MAXSHORTEXPOSURE"`;
          sed -i "s/$prodname = .*/$prodname = 0/g" $pos_file ;
      done
 done


for i in 123805 123806 123807 123808 ; do /home/pengine/prod/live_execs/user_msg --traderid $i --setmaxpos 1 ; done

