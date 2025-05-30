#!/bin/bash
declare -A dict

get_expiry_date () {
  ONE=01;
  EXPIRY=$date;
  for i in {1..7}; do dateStr=`date -d "$YYYY$MM$ONE + 1 month - $i day" +"%w %Y%m%d"`; if [ ${dateStr:0:1} -eq 4 ] ; then EXPIRY=${dateStr:2}; fi ; done
}

if [ $# -ne 1 ] ; then
  echo "Called As : " $* ;
  echo "$0 YYYYMMDD" ;
  exit ;
fi
date=$1
YYYY=${date:0:4}
MM=${date:4:2}
yyyy=${date:0:4}
mm=${date:4:2}
dd=${date:6:2}

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
      exit;
fi

next_working_day=`/home/pengine/prod/live_execs/update_date $date N A`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
while [[ $is_holiday = "1" ]]
do
     next_working_day=`/home/pengine/prod/live_execs/update_date $next_working_day N A`
     is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
done

ssh dvcinfra@10.23.5.69 "/home/pengine/prod/live_scripts/sync_all_midterm_simulation_data.sh"
echo "Running Preprocess BhavCopy"
cd /home/dvctrader/MFT_Analysis
/apps/anaconda/anaconda3/envs/py3/bin/python /home/dvctrader/MFT_Analysis/Analysis/KSRatio/bhavcopy_preprocess.py --startdate $date --enddate $date 
OI_file="/spare/local/tradeinfo/NSE_Files/OI_Files/$date"
if [[ ! -f $OI_file ]]; then
  echo "KSRatio OI file Does not Exist "
  echo " " | mailx -s "KSRatio OI file Does not Exist : ${date}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in nishit.bhandari@tworoads.co.in 
  exit
fi


echo "Running Settlement Prices"
/apps/anaconda/anaconda3/envs/py3/bin/python /home/dvctrader/MFT_Analysis/Analysis/KSRatio/settlement_prices.py

adjust_file="/home/dvctrader/AnalysisOut/KSRatio/Price Data/adjusted_settlement_prices_debug.csv"
count=`grep $date "$adjust_file" | wc -l`
echo "Ajdust Count: $count"
if [[ $count -lt 10 ]] ; then
    echo "KsRatio Ajdust Price Missing "
  echo "" | mailx -s "KsRatio Ajdust Price Missing : ${date}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in nishit.bhandari@tworoads.co.in 
  exit
fi
echo "Generate Beta Sigma"
/apps/anaconda/anaconda3/envs/py3/bin/python /home/dvctrader/MFT_Analysis/Analysis/KSRatio/Generate_Beta_Sigma.py --enddate $date
settlePrice="/home/dvctrader/AnalysisOut/KSRatio/Preprocess Outputs/Preprocess%settlement%250%250%1.csv"
count=`grep $date "$settlePrice" | wc -l`
echo "Settlement Count: $count"
if [[ $count -lt 10 ]] ; then
    echo "KsRatio Settle Price Missing "
  echo "" | mailx -s "KsRatio Settle Price Missing : ${date}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in nishit.bhandari@tworoads.co.in 
  exit
fi

ksOut=/spare/local/MidtermStrat/KSRatio/$next_working_day
mkdir /spare/local/MidtermStrat/KSRatio
grep $date "$settlePrice" | awk -F ',' 'BEGIN{OFS=","} {print "SIGMA_BETA_NSE_"$3"_FUT0 = "$5,$4}' > $ksOut

echo "Running Analysis KSRatio $next_working_day"
mkdir -p /home/dvctrader/AnalysisOut/KSRatio/Outputs/1111
rm -rf /home/dvctrader/AnalysisOut/KSRatio/Outputs/1112
/apps/anaconda/anaconda3/envs/py3/bin/python /home/dvctrader/MFT_Analysis/Analysis/KSRatio/KSRatio_analysis.py --enddate $next_working_day


log="/home/dvctrader/AnalysisOut/KSRatio/Outputs/1112/logs"
if [[ ! -f $log ]]; then
  echo "Log File Does not Exist "
  echo " " | mailx -s "Log file Does not Exist : ${date}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in nishit.bhandari@tworoads.co.in 
  exit
fi
count=`grep $next_working_day $log | wc -l`
echo "Log Count: $count"
if [[ $count -lt 1 ]] ; then
    echo "Log File Missing $date"
  echo "" | mailx -s "Log File Count Missing : ${date}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.innishit.bhandari@tworoads.co.in 
  exit
fi


grep -A1 "Todays Selection: $next_working_day" $log | tail -1 | sed "s/(\['/(\['NSE_/g" | sed "s/'\]/_FUT0'\]/g" | sed "s/', /_FUT0', /g" | sed "s/, '/, 'NSE_/g" | cut -d ']' -f1 | sed "s/': Index(\[/=/g" | sed "s/{'//g" | sed "s/'//g" | sed 's/ //g' | awk -F"=" '{print "ASSET_"toupper($1)" = "$2}' >> $ksOut
grep -A1 "Todays Selection: $next_working_day" $log | tail -1 | sed "s/(\['/(\['NSE_/g" | sed "s/'\]/_FUT0'\]/g" | sed "s/', /_FUT0', /g" | sed "s/, '/, 'NSE_/g" | cut -d ']' -f2 | cut -d ')' -f2 | sed "s/': Index(\[/=/g" | sed "s/{'//g" | sed "s/'//g" | cut -d',' -f2- | sed 's/^ *//g' | sed "s/'//g" | sed 's/ //g' | awk -F"=" '{print "ASSET_"toupper($1)" = "$2}' |  sed 's/ASSET_NSE_/ASSET_/g' >> $ksOut

tmp_ksratio="/tmp/agree_tmp_details_lookup"
cat $log | grep AGGRE_ | awk -F ',' '{h[$1] += $2;b[$1] = $3; next} END { for (i in h) {print i,"=",h[i]","b[i]}}' > $tmp_ksratio

lot_file="/spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_${date}.csv"
cat $lot_file | awk -F',' '{print $2,$3}' | awk '{print "AGGRE_NSE_"$1"_FUT0",$2}' >/tmp/lot_size_69

while IFS=' ' read -r key value; do
    dict[$key]=$value
done < /tmp/lot_size_69
tmp_1="/tmp/agree_tmp_details_lookup_temp_2_files"
grep AGGRE_NSE_ $tmp_ksratio | tr ',' ' ' >$tmp_1
while IFS=' ' read -r f1 f2 f3 f4; do
  tmp_=$(($f3 * ${dict[$f1]}))
#####  echo "$f1 = $tmp_,$f4" >>$ksOut
done < $tmp_1

'''
echo "Running Analysis KSRatio $date"
mkdir -p /home/dvctrader/AnalysisOut/KSRatio/Outputs/1111
rm -rf /home/dvctrader/AnalysisOut/KSRatio/Outputs/1112
/apps/anaconda/anaconda3/envs/py3/bin/python /home/dvctrader/MFT_Analysis/Analysis/KSRatio/KSRatio_analysis.py --enddate $date

log="/home/dvctrader/AnalysisOut/KSRatio/Outputs/1112/logs"
if [[ ! -f $log ]]; then
  echo "Log File Does not Exist "
  echo " " | mailx -s "Log file Does not Exist : ${date}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
  exit
fi
count=`grep $date $log | wc -l`
echo "Log Count: $count"
if [[ $count -lt 1 ]] ; then
    echo "Log File Missing $date"
  echo "" | mailx -s "Log File Count Missing : ${date}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
  exit
fi

cat $log | grep ALGO_  >> $ksOut
'''


count=`grep ALGO_ $ksOut | wc -l`
count=20
count2=`grep ASSET_LONG $ksOut | wc -l`
count3=`grep AGGRE_ $ksOut | wc -l`
count3=20
echo "Algo Count: $count Long: $count2 AGGRE: $count3"
if [[ $count -lt 1 || $count2 -ne 1 || $count3 -lt 1 ]] ; then
    echo "Algo Count Missing "
  echo "" | mailx -s "Asset Count Missing : ${date}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in nishit.bhandari@tworoads.co.in 
  exit
fi

rsync -avz /spare/local/MidtermStrat/ 52.90.0.239:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.64:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.65:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.69:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.84:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.72:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.63:/spare/local/MidtermStrat 


echo "" | mailx -s "KsRatio Daily File Generated: ${date}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in nishit.bhandari@tworoads.co.in 
/home/pengine/prod//live_scripts/daily_Jobs_KsRatio_algo_aggre.sh $date

get_expiry_date;
echo "Expiry : $EXPIRY";
EXPIRY=`/home/pengine/prod/live_execs/update_date $EXPIRY P W`
is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $EXPIRY T`
## Thursday
if [[ $EXPIRY == $date ]]; then
        echo "" | mailx -s "REMINDER: Please Update Name FUT1 to FUT0 KSRatio for Tomorrow $date" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in nishit.bhandari@tworoads.co.in infra_alerts@tworoads-trading.co.in
  $slack_exec $slack_channel DATA "*${HOSTNAME}-${USER}*\nREMINDER: Please Adjust KsRatio For Tomorrow $date\n"
fi

## Wednesday Night
while [ $is_holiday_ = "1" ];
do
    EXPIRY=`/home/pengine/prod/live_execs/update_date $EXPIRY P W`
    is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $EXPIRY T`
done
## Tuesday Night
while [ $is_holiday_ = "1" ];
do
    EXPIRY=`/home/pengine/prod/live_execs/update_date $EXPIRY P W`
    is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $EXPIRY T`
done

slack_exec="/home/pengine/prod//live_execs/send_slack_notification"
slack_channel="mail-service"
echo "day before expiry: $EXPIRY"
if [[ $EXPIRY == $date ]]; then
        echo "" | mailx -s "REMINDER: Please Adjust KsRatio and SIMR For Tomorrow $date" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in nishit.bhandari@tworoads.co.in infra_alerts@tworoads-trading.co.in
  $slack_exec $slack_channel DATA "*${HOSTNAME}-${USER}*\nREMINDER: Please Adjust KsRatio For Tomorrow $date\n"
fi



echo "Script is Completed..."
echo "Sleeping For 100minutes, and will try sync again in case of line down"
sleep 100m
rsync -avz /spare/local/MidtermStrat/ 52.90.0.239:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.64:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.65:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.69:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.84:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.72:/spare/local/MidtermStrat
rsync -avz /spare/local/MidtermStrat/ 10.23.227.63:/spare/local/MidtermStrat
