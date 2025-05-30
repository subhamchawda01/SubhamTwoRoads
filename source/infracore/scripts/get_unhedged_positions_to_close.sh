#!/bin/bash

date=$1
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi

today_date_=$1;
get_shortcode_exec_="/home/pengine/prod/live_execs/get_shortcode_for_symbol"
prev_date_=`date -d "$today_date_ 1 day ago" +\%Y\%m\%d`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $prev_date_ T`


while [ $is_holiday = "1" ] ;
do
   i=$(( i + 1))
   prev_date_=`date -d "$today_date_ $i day ago" +\%Y\%m\%d`
   is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $prev_date_ T`
done

OPTION_POS_FILE_="/tmp/options_pos_file_${today_date_}"
TMP_OPT_LOT_FILE="/tmp/options_lots_file_${today_date_}"
TMP_FUT_LOT_FILE="/tmp/fut_lots_file_${today_date_}"
FUT_POS_FILE_="/tmp/fut_pos_file_${today_date_}"
SEC_UNDERBAN_OPTION_POS_FILE_="/tmp/sec_underban_options_pos_file_${today_date_}"
SEC_UNDERBAN_FUT_POS_FILE_="/tmp/sec_underban_fut_pos_file_${today_date_}"
ADDTS_FILE_="/tmp/unhedgedged_pos_addts_${today_date_}"
SEC_UNDERBAN_FILE_="/spare/local/tradeinfo/NSE_Files/SecuritiesUnderBan/fo_secban_${today_date_}.csv"
unh_no_addts_file="/tmp/unhgd_prod_no_addts_file_${today_date_}"
adjust_addts_file_="/tmp/unheged_pos_adjust_file_${today_date_}"
>$adjust_addts_file_
>$FUT_POS_FILE_
>$OPTION_POS_FILE_
>$TMP_OPT_LOT_FILE
>$TMP_FUT_LOT_FILE
>$ADDTS_FILE_
>$SEC_UNDERBAN_OPTION_POS_FILE_
>$SEC_UNDERBAN_FUT_POS_FILE_
>$unh_no_addts_file
#
#get lot size for fo products
prod_to_lotsize='/tmp/prod_to_lotsize'
trade_info_dir='/spare/local/tradeinfo/NSE_Files/'
lotsize_file=${trade_info_dir}'/Lotsizes/fo_mktlots_'${today_date_}'.csv'
echo "LOTSIZE_FILE: $lotsize_file"
cat ${lotsize_file} | grep -v SYMBOL \
    | awk -F "," '{gsub(" ","",$2);print $2"_0",$3,"\n"$2"_1",$4"\n"$2"_2",$5}' > ${prod_to_lotsize}

declare -A prod_to_lotsize_map

while read -r line;
do
  prod=`echo ${line} | awk '{print $1}'`;
  lotsize=`echo ${line} | awk '{print $2}'`;
  prod_to_lotsize_map[$prod]=${lotsize}
done  < ${prod_to_lotsize}
#

echo "prev date: $prev_date_"
echo "FUT Positions:" ; 
 for prod in `cat /NAS1/data/MFGlobalTrades/ind_pnls/FO/eod_pnls/ind_pnl_${prev_date_}.txt | grep FUT | awk '{print $2;}' | cut -d'_' -f1 | sort | uniq` ; 
 do 
   lot=`cat "/spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_${today_date_}.csv" | awk -v prod=${prod} -F ',' '{gsub(/[ \t]+$/, "", $2); if($2 == prod) {print $3;}}'` ; 
   grep " ${prod}_FUT" /NAS1/data/MFGlobalTrades/ind_pnls/FO/eod_pnls/ind_pnl_${prev_date_}.txt | sort | awk -v prod="$prod" '{pos += $(NF-6)} END {if(pos != 0) {printf "%s %d ",prod,pos;}}'; 
   echo $lot ;
 done | awk '{if (NF > 1 && (($2/$3) > 0.7 || ($2/$3) < -0.7 )) {print "NSE_"$1"_FUT0",$3,-$2/$3;}}' | sort -nrk3 | awk 'function abs(v) { return v < 0 ? -v : v } function ceil(v){ abs_v=abs(v); v_lu_mid=int(abs_v)+0.5; if(abs_v>v_lu_mid){u_vi= int(abs_v)+1; return v < 0 ? -u_vi : u_vi} else{l_vi= int(abs_v); return v < 0 ? -l_vi : l_vi } } {print $1,$2*ceil($3)}' | sort -nrk2 >$FUT_POS_FILE_;
	 
	 #awk 'function abs(v) { return v < 0 ? -v : v } function ceil(v){ abs_v=abs(v); v_i=int(abs_v); if(abs_v > v_i){v_i=v_i+1} if(v>0){return v_i;} else{return -v_i;}; } {print $1,$2*ceil($3)}' | sort -nrk2 >$FUT_POS_FILE_;
 echo "" ; 

 if [ -f $FUT_POS_FILE_ ];
 then 
   while read -r line;
   do
     sym=`echo ${line} | awk '{print $1}'`;
     limit=`echo ${line} | awk '{print $2}'`;
     sym_type=`echo $sym | cut -d '_' -f3`
     basename_exp_num=`echo $sym | cut -d '_' -f2`"_"${sym_type: -1}
#     [ -z ${prod_to_lotsize_map[$basename_exp_num]} ] && continue;
#     [ -z ${limit} ] && continue;
     if [ -z ${prod_to_lotsize_map[$basename_exp_num]} ] || [ -z ${limit} ];
     then 
       echo "***UNHEDGED: $line"
       echo "$sym $limit" >> $unh_no_addts_file
       continue;
     fi

     lot_sz=${prod_to_lotsize_map[$basename_exp_num]}
     limit_new=$(( (${limit} + ${lot_sz} - 1 )/ ${lot_sz} ));
#     echo "NSE MSFO7 ADDTRADINGSYMBOL $sym $limit_new $limit_new $limit_new $limit_new" >> $ADDTS_FILE_
     echo "$sym $limit_new" >> $TMP_FUT_LOT_FILE
   done < $FUT_POS_FILE_
 fi 
 awk '{if($2>20){val=$2;}else{val=20} print "NSE MSFO7 ADDTRADINGSYMBOL \""$1"\" "val,val,val,val }' $TMP_FUT_LOT_FILE >> $ADDTS_FILE_
 echo "ADDTS:FILE: $ADDTS_FILE_"
 echo "Unhedged products NOT added in ADDTS FILE: $unh_no_addts_file"
 cat $unh_no_addts_file

 echo "OPT Positions:" ; 
 
#for line in `egrep "C0|P0" /NAS1/data/MFGlobalTrades/ind_pnls/FO/eod_pnls/ind_pnl_${prev_date_}.txt | grep -v "TOTAL_POS:      0" | awk '{printf "%s,%d\n",$(NF-1),-$(NF-6)}'`;
for line in `egrep "C0|P0" /NAS1/data/MFGlobalTrades/ind_pnls/FO/eod_pnls/ind_pnl_${prev_date_}.txt | grep -v "TOTAL_POS:      0" | awk 'function abs(v) { return v < 0 ? -v : v } {if(abs($(NF-3))>20){ val=abs($(NF-3));}else{ val=20; } printf "%s,%d,%d\n",$(NF-1),-$(NF-6),val}'`;
#egrep "C0|P0" /NAS1/data/MFGlobalTrades/ind_pnls/FO/eod_pnls/ind_pnl_${prev_date_}.txt | grep -v "TOTAL_POS:      0" | awk '{printf "%s,%d\n",$(NF-1),$(NF-6)}' >/tmp/pos_file_${today_date_}
#for line in `cat /tmp/pos_file_${today_date_}`;
 do
#echo "LINE:: $line"
   old_exch_sym_=`echo $line | awk -F, '{print $1}'`
   pos_=`echo $line | awk -F, '{print $2}'`
   lots_=`echo $line | awk -F, '{print $3}'`
#echo "--> $old_exch_sym_ $pos_"
   new_shortcode_=`$get_shortcode_exec_ $old_exch_sym_ $today_date_`
   data_source_content_=`grep -w $old_exch_sym_ /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt | awk -F_ '{print $NF}'`
   echo "$new_shortcode_ $pos_ $data_source_content_" >> $OPTION_POS_FILE_
   echo "$new_shortcode_ $lots_" >> $TMP_OPT_LOT_FILE
 done

awk '{print "NSE MSFO7 ADDTRADINGSYMBOL \""$1"\" "$2,$2,$2,$2}' $TMP_OPT_LOT_FILE >> $ADDTS_FILE_

sed -i 's/~/\&/g' "${FUT_POS_FILE_}"
sed -i 's/~/\&/g' "${OPTION_POS_FILE_}"
sed -i 's/~/\&/g' "${ADDTS_FILE_}"

awk -F'"' '{print $2,$3}' $ADDTS_FILE_ | awk '{print $1,$2}' >$adjust_addts_file_
echo "adjust addts pos file: $adjust_addts_file_"

#seperate file for security under ban

awk '{print "_"$1"_"}' ${SEC_UNDERBAN_FILE_} >/tmp/test_sec; 
grep -f /tmp/test_sec ${FUT_POS_FILE_} | sort -k1 > ${SEC_UNDERBAN_FUT_POS_FILE_}
grep -f /tmp/test_sec ${OPTION_POS_FILE_} | sort -k1 > ${SEC_UNDERBAN_OPTION_POS_FILE_}

comm -3 <(sort -k1 ${FUT_POS_FILE_}) ${SEC_UNDERBAN_FUT_POS_FILE_} | grep ^NSE >/tmp/fut_pos_file_temp
mv /tmp/fut_pos_file_temp ${FUT_POS_FILE_}

comm -3 <(sort -k1 ${OPTION_POS_FILE_}) ${SEC_UNDERBAN_OPTION_POS_FILE_} | grep ^NSE >/tmp/options_pos_file_temp
mv /tmp/options_pos_file_temp ${OPTION_POS_FILE_}

#

error=0
scp $OPTION_POS_FILE_ dvctrader@10.23.227.64:~/ATHENA/
if [[ $? != 0 ]]; then
  error=1
  echo "CMD_Failed: scp $OPTION_POS_FILE_ dvctrader@10.23.227.64:~/ATHENA/"| mail -s "SYNC FAILED For Unhedged Positions : ${today_date_}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
fi
scp $OPTION_POS_FILE_ dvctrader@10.23.227.65:~/ATHENA/

scp $FUT_POS_FILE_ dvctrader@10.23.227.64:~/ATHENA/
if [[ $? != 0 ]]; then
  error=1
  echo "CMD_Failed: scp $FUT_POS_FILE_ dvctrader@10.23.227.64:~/ATHENA/"| mail -s "SYNC FAILED For Unhedged Positions : ${today_date_}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
fi
scp $FUT_POS_FILE_ dvctrader@10.23.227.65:~/ATHENA/

scp $SEC_UNDERBAN_FUT_POS_FILE_ dvctrader@10.23.227.64:~/ATHENA/
if [[ $? != 0 ]]; then
  error=1
  echo "CMD_Failed: scp $SEC_UNDERBAN_FUT_POS_FILE_ dvctrader@10.23.227.64:~/ATHENA/"| mail -s "SYNC FAILED For Unhedged Positions : ${today_date_}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
fi
scp $SEC_UNDERBAN_FUT_POS_FILE_ dvctrader@10.23.227.65:~/ATHENA/

scp $SEC_UNDERBAN_OPTION_POS_FILE_ dvctrader@10.23.227.64:~/ATHENA/
if [[ $? != 0 ]]; then
  error=1
  echo "CMD_Failed: scp $SEC_UNDERBAN_OPTION_POS_FILE_ dvctrader@10.23.227.64:~/ATHENA/"| mail -s "SYNC FAILED For Unhedged Positions : ${today_date_}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
fi
scp $SEC_UNDERBAN_OPTION_POS_FILE_ dvctrader@10.23.227.65:~/ATHENA/

scp $ADDTS_FILE_ dvcinfra@10.23.227.64:~/trash/
if [[ $? != 0 ]]; then
  error=1
  echo "CMD_Failed: scp $ADDTS_FILE_ dvcinfra@10.23.227.64:~/trash/"| mail -s "SYNC FAILED For Unhedged Positions : ${today_date_}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
fi
scp $ADDTS_FILE_ dvcinfra@10.23.227.65:~/trash/
if [[ $? != 0 ]]; then
  error=1
  echo "CMD_Failed: scp $ADDTS_FILE_ dvcinfra@10.23.227.65:~/trash/"| mail -s "SYNC FAILED For Unhedged Positions : ${today_date_}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
fi


scp $adjust_addts_file_ dvctrader@10.23.227.64:/tmp/
if [[ $? != 0 ]]; then
  error=1
  echo "CMD_Failed: scp $adjust_addts_file_ dvctrader@10.23.227.64:/tmp/"| mail -s "SYNC FAILED For Unhedged Positions : ${today_date_}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
fi
scp $adjust_addts_file_ dvctrader@10.23.227.65:/tmp/
if [[ $? != 0 ]]; then
  error=1
  echo "CMD_Failed: scp $adjust_addts_file_ dvctrader@10.23.227.65:/tmp/"| mail -s "SYNC FAILED For Unhedged Positions : ${today_date_}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
fi

#
ssh dvcinfra@10.23.227.64 "/home/pengine/prod/live_scripts/unhedged_prod_addts.sh ${today_date_}"
if [[ $? != 0 ]]; then
  error=1
  echo "CMD_Failed: ssh dvcinfra@10.23.227.64 /home/pengine/prod/live_scripts/unhedged_prod_addts.sh ${today_date_}" | mail -s "SYNC FAILED For Unhedged Positions : ${today_date_}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
fi

ssh dvcinfra@10.23.227.65 "/home/pengine/prod/live_scripts/unhedged_prod_addts.sh ${today_date_}"
if [[ $? != 0 ]]; then
  error=1
  echo "CMD_Failed: ssh dvcinfra@10.23.227.65 /home/pengine/prod/live_scripts/unhedged_prod_addts.sh ${today_date_}" | mail -s "SYNC FAILED For Unhedged Positions : ${today_date_}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
fi


if [[ $error == 0 ]]; then 
  echo "Unhedged Position file synced Successfully IND14" | mail -s "SYNC SUCCEED For Unhedged Positions : ${today_date_}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in  hardik.dhakate@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
  slack_exec="/home/pengine/prod//live_execs/send_slack_notification"
        slack_channel="mail-service"
        $slack_exec $slack_channel DATA "*${HOSTNAME}-${USER}*\nUnhedged Position file synced Successfully IND14\n"
fi
#
