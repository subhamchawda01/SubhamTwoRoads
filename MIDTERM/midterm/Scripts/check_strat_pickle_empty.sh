#!/bin/bash

dir_name="/spare/local/files/NSE/MidTermLogs/Strategy_Pickle_Dump"
mail_file="/tmp/alert_for_pickle_empty"
>$mail_file

month_=`date +%b%y`
pickle_to_check=("NRB" "Dispersion_${month_}_v1" "Dispersion_${month_}_v2" "RelMom" "BAS" "KSRatio" "RSM_LongTerm" "IntraSectorRV" "Dispersion_${month_}_v3" "Dispersion_${month_}_v4" "OSM_BNFW_2" "OSM_NFW_2" "OSM_NFW_1" "OSM_BNFW_1" "SIMR" "ShortGamma_NFW_1" "Pullbacks_NIFTY" "Pullbacks_BANKNIFTY" "ShortGamma_NFW_2" "ShortGamma_BNFW_2" "SSGapv2" "ShortGamma_BNFW_1" "GapTrader_BANKNIFTY" "GapTrader_NIFTY")


for str in ${pickle_to_check[@]}; do
      echo $dir_name/$str
      if [ ! -s $dir_name/$str ]
      then
               echo "File empty $dir_name/$str"
               echo "File empty $dir_name/$str" >>$mail_file
      fi
  done

if [ -s $mail_file ]
then
         echo "File not empty"
         cat $mail_file | mailx -s "Alert: Error Picke File empty" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in  smit@tworoads-trading.co.in  subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
fi
