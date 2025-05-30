#!/bin/bash

date=`date +\%Y\%m\%d`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi
OEBU_MAIL="/tmp/oebu_product_absence_mail"
PRODUCT_NOT_PRESENT_FILE="/tmp/prod_not_present"
CRONTAB_FILE="/tmp/crontab_files"
STRAT_CONFIG_FILE="/tmp/strat_config_file"
TEMP_POS_PROD_FILE="/tmp/pos_file_prod"
TEMP_PROD_FILE="/tmp/temp_prod"
true >$OEBU_MAIL

#                ["IND12"]="10.23.227.62" \
#                ["IND13"]="10.23.227.63" \
declare -A IND_Server_ip
  IND_Server_ip=( ["INDB11"]="192.168.132.11"
			["INDB12"]="192.168.132.12")

declare -A IND_Server_TMP_FILE
  IND_Server_TMP_FILE=( ["INDB11"]="/tmp/oebu_prod_tmp_indb11" \
                ["INDB12"]="/tmp/oebu_prod_tmp_indb12")

declare -A IND_Server_OEBU_FILE
  IND_Server_OEBU_FILE=( ["INDB11"]="/spare/local/files/oebu_volmon_product_list_indb11.txt" \
                ["INDB12"]="/spare/local/files/oebu_volmon_product_list_indb12.txt")


for server in "${!IND_Server_ip[@]}";
do
  echo "for ---- $server";
#if [ "$server" = "IND17" ]; then  #|| [ "$server" != "IND12" ] || [ "$server" != "IND13" ]; then
    true>${IND_Server_TMP_FILE[$server]}
    ssh dvctrader@${IND_Server_ip[$server]} "crontab -l | grep -v '^#' | grep run | awk '{if(NF == 6) print}'" >$CRONTAB_FILE
    #ssh ${IND_Server_ip[$server]} "crontab -l | grep -v '^#' | grep run">/tmp/crontab_files #backup line
    echo "SSH DONE CRON"
    files=`cat $CRONTAB_FILE | awk '{print $6}'`
    echo -e "for $server\n"

#run.sh files
    for file in $files;
    do
      echo -e "  $file->\n"
      ssh dvctrader@${IND_Server_ip[$server]} "cat $file | grep -v '^#' | grep -v 123421 | grep '/ \|CONFIG'| awk '{print \$2}'" >$STRAT_CONFIG_FILE 

#config files
      for config_file_ in `cat $STRAT_CONFIG_FILE`;
      do
         echo "    config_file:: $config_file_"
         ssh dvctrader@${IND_Server_ip[$server]} " grep THEO $config_file_ | awk '{print \$3}' | sed 's/_MM$//g;s/_SQUAREOFF$//g;s/^HDG_//g' | sort | uniq" >$TEMP_PROD_FILE
#pos_file=$(dirname "${config_file_}")"/PositionLimits.csv"
#echo $pos_file
         #ssh dvctrader@${IND_Server_ip[$server]} " grep NSE $pos_file | cut -d' ' -f1 | sed 's/_MAXLONGEXPOSURE$//g;s/_MAXSHORTEXPOSURE$//g;s/_MAXLONGPOS$//g;s/_MAXSHORTPOS$//g' | sort -u " >${TEMP_POS_PROD_FILE}
#cat $TEMP_POS_PROD_FILE >> ${IND_Server_TMP_FILE[$server]}
          cat $TEMP_PROD_FILE >> ${IND_Server_TMP_FILE[$server]}
#         for prods in `cat $TEMP_PROD_FILE`;
#         do
#              if grep -q $prods $TEMP_POS_PROD_FILE; then
#              echo "$prods" >> ${IND_Server_TMP_FILE[$server]}
#              fi
#         done
      done
    done
    sort ${IND_Server_TMP_FILE[$server]} | uniq >/tmp/oebu_file
    mv /tmp/oebu_file ${IND_Server_TMP_FILE[$server]}

  true >$PRODUCT_NOT_PRESENT_FILE
    echo "for :: ${IND_Server_TMP_FILE[$server]}"
  for prod in `cat ${IND_Server_TMP_FILE[$server]}`; do
#echo "$prod" 
    if [ `grep -w $prod ${IND_Server_OEBU_FILE[$server]} | wc -l` == "0" ]; then 
      echo "$prod not present"
      echo "$prod" >>$PRODUCT_NOT_PRESENT_FILE; 
    fi 
  done

  if [ `cat $PRODUCT_NOT_PRESENT_FILE | wc -l` -gt 0 ]; then
    echo -e "\n====== $server ======" >> $OEBU_MAIL;
    echo -e "FILE: ${IND_Server_OEBU_FILE[$server]} \n" >> $OEBU_MAIL;
    cat $PRODUCT_NOT_PRESENT_FILE >> $OEBU_MAIL;
  fi

#  cat ${IND_Server_OEBU_FILE[$server]} >/tmp/cp_oebu_file
  cat ${IND_Server_TMP_FILE[$server]} >/tmp/cp_oebu_file
#if [ "$server" = "IND19" ] || [ "$server" = "IND20" ];
  if [ "$server" = "IND14" ]; # || [ "$server" = "IND20" ];
  then
    sort -u /tmp/cp_oebu_file | grep -v NIFTY > ${IND_Server_OEBU_FILE[$server]}
  else
    sort -u /tmp/cp_oebu_file > ${IND_Server_OEBU_FILE[$server]}
  fi
#  fi
done

if [ `cat $OEBU_MAIL | wc -l` -gt 0 ]; then
  echo "sending mail"
  cat $OEBU_MAIL | mailx -s "=== BSE CASH OEBU PRODUCT NEEDS TO UPDATE ===" -r "${HOSTNAME}-${USER}<hardik.dhakate@tworoads-trading.co.in>" hardik.dhakate@tworoads-trading.co.in , raghunandan.sharma@tworoads-trading.co.in , subham.chawda@tworoads-trading.co.in #, ravi.parikh@tworoads.co.in, uttkarsh.sarraf@tworoads.co.in, nishit.bhandari@tworoads-trading.co.in, sanjeev.kumar@tworoads-trading.co.in
else
  echo "" | mailx -s "=== BSE CASH OEBU PRODUCT NEEDS TO UPDATE ===" -r "${HOSTNAME}-${USER}<hardik.dhakate@tworoads-trading.co.in>" hardik.dhakate@tworoads-trading.co.in , raghunandan.sharma@tworoads-trading.co.in , subham.chawda@tworoads-trading.co.in
fi
