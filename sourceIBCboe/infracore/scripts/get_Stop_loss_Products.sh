#!/bin/bash
stop_loss_tmp="/tmp/stop_loss_data.log"
mail_stop_loss="/tmp/stop_loss_data_mail"
mail_risk_check="/tmp/risk_check_data_mail"
rm -rf $mail_stop_loss
rm -rf $mail_risk_check

print_msg_and_exit(){
 echo $1
 exit
}

init () {
  [ $# -gt 0 ] || print_msg_and_exit "Usage : < script > < date >"
  currdate=$1;
  is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $currdate T`
  if [ $is_holiday = "1" ] ; then
      echo "NSE Holiday. Exiting...";
      exit;
  fi

  servers="IND11 IND12 IND13 IND14 IND15 IND16 IND17 IND18 IND22 IND19 IND20 IND23"
  declare -A IND_Server_ip
  IND_Server_ip=( ["IND11"]="10.23.227.61" \
                ["IND12"]="10.23.227.62" \
                ["IND13"]="10.23.227.63" \
                ["IND14"]="10.23.227.64" \
                ["IND15"]="10.23.227.65" \
                ["IND16"]="10.23.227.81" \
                ["IND17"]="10.23.227.82" \
                ["IND18"]="10.23.227.83" \
                ["IND19"]="10.23.227.69" \
		        ["IND22"]="10.23.227.71" \
                ["IND23"]="10.23.227.72" \
                ["IND20"]="10.23.227.84")

  for server in "${!IND_Server_ip[@]}";
  do
    if [ "$server" == "IND11" ] || [ "$server" == "IND12" ] || [ "$server" == "IND13" ] ; then #|| [ "$server" == "IND14" ] || [ "$server" == "IND16" ] || [ "$server" == "IND17" ] || [ "$server" == "IND18" ] || [ "$server" == "IND19" ] || [ "$server" == "IND20" ] ; then
      #echo -e "$server continue\n"
      continue;
    fi
    true>/tmp/STOP_LOSS_ENTRY
    true>/tmp/STOP_LOSS_PRODUCTS
    echo -e "--------------------------------------$server----------------------------------------\n";
    for file in `ssh dvcinfra@${IND_Server_ip[$server]} "ls '/spare/local/logs/tradelogs/' | grep 'log.'$currdate"`;
    do
      stratid=$(echo $file| cut -d'.' -f 3);
#echo "***file, strat:: $file, $stratid"
      ssh dvcinfra@${IND_Server_ip[$server]} "zgrep -i 'STOP LOSS' '/spare/local/logs/tradelogs/log.'$currdate'.'$stratid | grep -v 'BarUpdate\|TENTATIVE' | awk -v strat_id=$stratid '{print \$0,strat_id}'" >>/tmp/STOP_LOSS_ENTRY

#To check Risk Alert
      ssh dvcinfra@${IND_Server_ip[$server]} "zgrep -i 'RISK CHECKS' '/spare/local/logs/tradelogs/log.'$currdate'.'$stratid | awk '{print \$1,\$3,\$4,\$5,\$10,\$11,\$12,\$13,\$14,\$15,\$16,\$17,\$18,\$19}' | tail -1 | grep HIT" >>/tmp/RISK_CHECK_${currdate}.${stratid}
#
      while read -r line
      do
        if ! grep -Fq "$line" /tmp/risk_check_tmp_${currdate}.${stratid}
        then
          echo "$server $line" >> /tmp/risk_check_tmp_${currdate}.${stratid}
          time_stamp=`echo "$line"| cut -d' ' -f1`
          time_t=`date -d"@$time_stamp"`
          time_=`date -d "$time_t +330 minutes" +"%r"`
          pnl_=`echo "$line"| cut -d' ' -f6`
          stop_loss_=`echo "$line"| cut -d' ' -f8`
          total_GE_=`echo "$line"| cut -d' ' -f12`
          GE_=`echo "$line"| cut -d' ' -f14`
          echo "Server:- $server Strat_ID:- $stratid Time:- $time_ PNL:- $pnl_ SL:- $stop_loss_ TOTAL_GE:- $total_GE_ GE:- $GE_" >> $mail_risk_check 
        fi
      done < /tmp/RISK_CHECK_${currdate}.${stratid}

#
    done
    STOP_LOSS_PRODUCTS=`awk '{if($2=="HITTING" && $3=="STOP")print $5; else if ($2=="HITTING" && $3=="HARD")print $6; else if ($3=="HITTING" && $4=="STOP")print $8; else if ($3=="HITTING" && $4=="HARD")print $9;}' /tmp/STOP_LOSS_ENTRY | sort | uniq`
#   echo -e "STOP_LOSS_PRODUCTS= \n $STOP_LOSS_PRODUCTS"
    for prod in $STOP_LOSS_PRODUCTS;
    do
#      echo -e "$prod :: \n `cat /tmp/STOP_LOSS_ENTRY | grep $prod`"
      grep $prod /tmp/STOP_LOSS_ENTRY | tail -1 | grep "HITTING">>/tmp/STOP_LOSS_PRODUCTS
    done 
#    echo "###PROD"
    while read -r line
    do
#        current_tmstp=`date +"%s"`
#        stop_loss_tmstp=`echo $line | cut -d' ' -f1| cut -d'.' -f1`
#        [ $(( current_tmstp - stop_loss_tmstp )) -lt 3600 ] && continue;
        if ! grep -Fq "$line" $stop_loss_tmp
        then
                echo "$server $line" >> $stop_loss_tmp
    #            if  [ `date +"%H%M"` -gt 0410 ] && [ $server == "IND17" ];then
    #                    continue;
    #            fi
                no_field=`echo "$line"| awk '{print NF}'`
                if [[ $no_field -eq 20 ]]; then 
                    strat_id=`echo "$line"| rev | cut -d' ' -f1 | rev`
                    time_stamp=`echo "$line"| cut -d' ' -f1`
                    time_t=`date -d"@$time_stamp"`
                    time_=`date -d "$time_t +330 minutes" +"%r"`
                    sym=`echo "$line"| cut -d' ' -f8`
                    pnl=`echo "$line"| cut -d' ' -f10`
                    stop_loss=`echo "$line"| cut -d' ' -f14`
                    hard_stop=`echo "$line"| cut -d' ' -f16`
                    echo "Server:-   $server     Strat_ID:-   $strat_id     Type:-   NORMAL     Time:-   $time_     Symbol:-   $sym     PNL:-   $pnl    StopLoss:-   $stop_loss    HardLimit:-     $hard_stop" >> $mail_stop_loss
                elif [[ $no_field -eq 21 ]]; then
                    strat_id=`echo "$line"| rev | cut -d' ' -f1 | rev`
                    time_stamp=`echo "$line"| cut -d' ' -f1`
                    time_t=`date -d"@$time_stamp"`
                    time_=`date -d "$time_t +330 minutes" +"%r"`
                    sym=`echo "$line"| cut -d' ' -f9`
                    pnl=`echo "$line"| cut -d' ' -f11`
                    stop_loss=`echo "$line"| cut -d' ' -f15`
                    hard_stop=`echo "$line"| cut -d' ' -f17`
                    echo "Server:-   $server     Strat_ID:-   $strat_id     Type:-   HARD     Time:-   $time_     Symbol:-   $sym     PNL:-   $pnl    StopLoss:-   $stop_loss    HardLimit:-     $hard_stop" >> $mail_stop_loss
                elif [[ $no_field -eq 17 ]]; then
                    strat_id=`echo "$line"| rev | cut -d' ' -f1 | rev`
                    time_stamp=`echo "$line"| cut -d' ' -f1`
                    time_t=`date -d"@$time_stamp"`
                    time_=`date -d "$time_t +330 minutes" +"%r"`
                    sym=`echo "$line"| cut -d' ' -f6`
                    pnl=`echo "$line"| cut -d' ' -f8`
                    stop_loss=`echo "$line"| cut -d' ' -f10`
                    echo "Server:-   $server     Strat_ID:-   $strat_id     Type:-   HARD     Time:-   $time_     Symbol:-   $sym     PNL:-   $pnl    StopLoss:-   $stop_loss    HardLimit:-  ****" >> $mail_stop_loss
                else
                    strat_id=`echo "$line"| rev | cut -d' ' -f1 | rev`
                    time_stamp=`echo "$line"| cut -d' ' -f1`
                    time_t=`date -d"@$time_stamp"`
                    time_=`date -d "$time_t +330 minutes" +"%r"`
                    sym=`echo "$line"| cut -d' ' -f5`
                    pnl=`echo "$line"| cut -d' ' -f7`
                    stop_loss=`echo "$line"| cut -d' ' -f9`
                    echo "Server:-   $server     Strat_ID:-   $strat_id     Type:-   NORMAL     Time:-   $time_     Symbol:-   $sym     PNL:-   $pnl   StopLoss:-   $stop_loss    HardLimit:-  ****" >> $mail_stop_loss
                fi
        fi
    done < /tmp/STOP_LOSS_PRODUCTS
    cat /tmp/STOP_LOSS_PRODUCTS
    echo -e "\n-----------------------------------------------------------------------------------\n";
  done
  if [ -f $mail_risk_check ]; then
    echo "Risk check Alert present"
    echo -e "\n**** RISK CHECK ****" >>$mail_stop_loss
    cat $mail_risk_check >> $mail_stop_loss
  fi
  if [ -f $mail_stop_loss ]; then

        echo "Send Mail"
        cat $mail_stop_loss | sort -k4 |tac >/tmp/sortedTimeStopLoss
        mv /tmp/sortedTimeStopLoss $mail_stop_loss
        echo "" | mailx -s "Alert Stop Loss : ${currdate}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" taher.makda@tworoads-trading.co.in nishit.bhandari@tworoads.co.in raghunandan.sharma@tworoads-trading.co.in uttkarsh.sarraf@tworoads.co.in ravi.parikh@tworoads.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in < $mail_stop_loss
#        echo "" | mailx -s "Alert Stop Loss" -r "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>" subham.chawda@tworoads-trading.co.in < $mail_stop_loss
  fi
  rm /tmp/STOP_LOSS_ENTRY /tmp/STOP_LOSS_PRODUCTS
}

init $*
