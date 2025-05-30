#!/bin/bash

today=$1

yyyy=${today:0:4}
mm=${today:4:2}
dd=${today:6:2}
yy=${today:2:2}

is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today T`
if [ $is_holiday_ = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi

bhavcopy="/spare/local/tradeinfo/BSE_Files/Margin_Files/Exposure_Files/EQ${dd}${mm}${yy}.CSV"
symbolfile="/spare/local/tradeinfo/BSE_Files/RefData/bse_eq_${today}_contracts.txt"
dest_file="/spare/local/tradeinfo/BSE_Files/RefData/bse_groupAB_symbol_bhavcopy_$today"
>"$dest_file"
while IFS= read -r line;
do
   sc_group=`echo "$line"| awk -F, '{print $3}'`
   if [ $sc_group = 'A' ] || [ $sc_group = 'B' ]; then
      stk_code=`echo "$line" | awk -F, '{print $1}'`
      sc_symbol=`grep $stk_code $symbolfile | awk '{print $4}'`
      if [ ! -z "$sc_symbol" ]; then
         echo  "$sc_symbol, $line" >>"$dest_file"
      fi
   fi
done <"$bhavcopy"

