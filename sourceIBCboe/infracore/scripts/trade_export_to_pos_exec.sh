#!/bin/bash

print_msg_and_exit(){
  echo $* ;
  exit ;
}

GetNearestExpiry() {
  contract_file=${tradeinfo_dir}'/ContractFiles/nse_contracts.'${DATE}
  dt=`cat ${contract_file} | grep IDXFUT | grep BANKNIFTY | awk -v date=${DATE} -v no=${exp_no} '{if($NF>=date)print $NF'} | sort | uniq | head -n1 | tail -1`
  curr_exp_no=${dt}
  dt=`date -d"$dt" +%d%b%Y`
  curr_exp_date=`echo ${dt^^}`

  dt=`cat ${contract_file} | grep IDXFUT | grep BANKNIFTY | awk -v date=${DATE} '{if($NF>=date)print $NF'} | sort | uniq | head -n2 | tail -1`
  next_exp_no=${dt}
  dt=`date -d"$dt" +%d%b%Y`
  next_exp_date=`echo ${dt^^}`

  dt=`cat ${contract_file} | grep IDXFUT | grep BANKNIFTY | awk -v date=${DATE} '{if($NF>=date)print $NF'} | sort | uniq | head -n3 | tail -1`
  far_exp_no=${dt}
  dt=`date -d"$dt" +%d%b%Y`
  far_exp_date=`echo ${dt^^}`

#get current week expiry
  current_date=$DATE
  weekday=`date -d"$current_date" +%A`
  echo "weekday: $weekday"
  while [ "$weekday" != "Thursday" ] ;  
  do
    current_date=`date -d "$current_date tomorrow" +\%Y\%m\%d`
    weekday=`date -d"$current_date" +%A`
  done
  echo "cur_date: $current_date"

  is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $current_date T`
  while [ $is_holiday = "1" ] ;
  do
    current_date=`date -d "$current_date 1 day ago" +\%Y\%m\%d`
    is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $current_date T`
  done
  
  echo "cur_week_expiry: $current_date"
  curr_week_exp_no=$current_date
  dt=`date -d"$current_date" +%d%b%Y`
  curr_week_exp_date=`echo ${dt^^}`
}


compute_cm_pos_file(){
  echo "TYPE $TYPE"
  >/tmp/gen_pos_exec_
  echo "TRADE_EXPORT_FILE= $TRADE_EXPORT_FILE"
  echo "USERLIST_FILE= $USERLIST_FILE"
  #total_notional="BUY: 0 SELL: 0";
  for user_id in `cat $USERLIST_FILE`;
  do
    echo -e "******** USERID $user_id ************\n"
    awk -F, -v usr=${user_id} '{if($13==usr){ if($10=="B"){printf "%s %d %s %d %f\n",$5,1*$7,$10,$13,1*$9} else{ printf "%s %d %s %d %f\n",$5,-1*$7,$10,$13,-1*$9}}}' $TRADE_EXPORT_FILE >>/tmp/gen_pos_exec_
    echo -e "\t~~~~ LAST TRADE ~~~~"
    awk -F, -v usr=${user_id} '{if($13==usr) print $4,$5,$6,$7,$8,$10}' $TRADE_EXPORT_FILE | sort -rk1 | head -2
    echo -e "\t~~~~~~~~~~~~~~~~~~~~"
    #curr_not=`awk -F, -v usr=${user_id} '{if($13==usr) print $9,$10}' $TRADE_EXPORT_FILE | awk '{if($2=="B"){bsum+=$1} else {ssum+=$1}} END {printf "BUY: %f SELL: %f",bsum,ssum}' | head -1 `
    #total_not=$total_notional
    #total_notional=`echo "$curr_not $total_not" | awk '{printf "BUY: %f SELL: %f",$2+$6,$4+$8}'`
    #echo -e "\tNOTIONAL: $curr_not"
    echo -e "\n***********************************\n"
  done
  #cat /tmp/gen_pos_exec_ | awk '{prod_to_pos[$1]+=$2} END {for( key in prod_to_pos){ if(prod_to_pos[key] != 0) print "NSE_"key,-prod_to_pos[key]}}' > ${POS_EXEC_FILE}

  cat /tmp/gen_pos_exec_ | awk '{prod_to_pos[$1]+=$2; prod_to_not[$1]+=$(NF)} END {for( key in prod_to_pos){ if(prod_to_pos[key] != 0) print "NSE_"key,-prod_to_pos[key],prod_to_not[key]}}' >/tmp/prod_to_pos_not;
  awk '{print $1,$2}' /tmp/prod_to_pos_not > ${POS_EXEC_FILE}
  awk '{if($3>0){bsum+=$3} else {ssum+=$3}} END {printf "NOTIONAL: BUY: %f SELL: %f\n",bsum,ssum}' /tmp/prod_to_pos_not

  #echo "$total_notional" | awk '{printf "TOTAL NOTIONAL VALUE(Lac): BUY: %f SELL: %f\n",$2/100000,$4/100000}'
  echo "DONE"
}

compute_fo_pos_file(){
  

  echo "TYPE $TYPE"
  echo "TRADE_EXPORT_FILE= $TRADE_EXPORT_FILE"
  echo "USERLIST_FILE= $USERLIST_FILE"
  >/tmp/gen_pos_exec_
  >${POS_EXEC_FILE}
  >${FUT_POS_EXEC_FILE}
  >${OPT_POS_EXEC_FILE}
  GetNearestExpiry;
  echo -e "EXP0_W: $curr_week_exp_date EXP0: $curr_exp_date\nEXP1: $next_exp_date\nEXP2: $far_exp_date\n"
  echo -e "EXP0_W: $curr_week_exp_no EXP0: $curr_exp_no\nEXP1: $next_exp_no\nEXP2: $far_exp_no\n"
#  curr_week_exp_date=18MAR2021
#  curr_week_exp_no=20210318
  #FUT
  
  for user_id in `cat $USERLIST_FILE`;
  do
    echo -e "******** FUT USERID $user_id ************\n"
    egrep ",FUTSTK,|,FUTIDX," $TRADE_EXPORT_FILE  | awk -F, -v usr=${user_id} '{ if($16==usr){ if($13=="B"){print $5,$6,$7,$10,$13,$11} else{ print $5,$6,$7,-1*$10,$13,$11}}}' | awk -v e1=${curr_exp_date} -v e2=${next_exp_date} -v e3=${far_exp_date} '{if($3==e1){f="_FUT0"}else if($3==e2){f="_FUT1"} else if($3==e3){f="_FUT2"} printf "NSE_%s%s %d %s %f\n",$2,f,$4,$5,$6}' >>/tmp/gen_pos_exec_
  
    echo -e "\t~~~~ LAST TRADE ~~~~"
    egrep ",FUTSTK,|,FUTIDX," $TRADE_EXPORT_FILE  | awk -F, -v usr=${user_id} '{ if($16==usr){ if($13=="B"){print $5,$6,$7,$10,$13,$4,$11,$12} else{ print $5,$6,$7,-1*$10,$13,$4,$11,$12}}}' | awk -v e1=${curr_exp_date} -v e2=${next_exp_date} -v e3=${far_exp_date} '{if($3==e1){f="_FUT0"}else if($3==e2){f="_FUT1"} else if($3==e3){f="_FUT2"} printf "%s NSE_%s%s %d %f %s\n",$6,$2,f,$4,$7,$5}' | sort -rk1 | head -2
    echo -e "\t~~~~~~~~~~~~~~~~~~~~"
    echo -e "\n***********************************\n"
  done
  cat /tmp/gen_pos_exec_ | awk '{pos[$1]+=$2} END {for(i in pos){ if(pos[i]!=0)  print i,pos[i]}}' >${FUT_POS_EXEC_FILE}

  #get unhedged postions

  for prod in `cat ${FUT_POS_EXEC_FILE} | awk '{print $1}' | cut -d'_' -f2 | sort -u`;
  do
    lot=`cat "/spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_${DATE}.csv" | awk -v prod=${prod} -F ',' '{gsub(/[ \t]+$/, "", $2); if($2 == prod) {print $3;}}'` ;
    grep "${prod}_FUT" ${FUT_POS_EXEC_FILE} | sort | awk -v prod="$prod" '{pos += $2} END {if(pos != 0) {printf "%s %d ",prod,pos;}}';
    echo $lot ;
  done | awk '{if (NF > 1 && (($2/$3) > 0.7 || ($2/$3) < -0.7 )) {print "NSE_"$1"_FUT0",$3,-$2/$3;}}' | sort -nrk3 | awk 'function abs(v) { return v < 0 ? -v : v } function ceil(v){ abs_v=abs(v); v_lu_mid=int(abs_v)+0.5; if(abs_v>v_lu_mid){u_vi= int(abs_v)+1; return v < 0 ? -u_vi : u_vi} else{l_vi= int(abs_v); return v < 0 ? -l_vi : l_vi } } {print $1,$2*ceil($3)}' | sort -nrk2 > ${POS_EXEC_FILE}


  #OPTION
  >/tmp/gen_opt_pos_exec_
  echo -e "\n*********** OPTIONS ************"
  for user_id in `cat $USERLIST_FILE | sort -u`;
  do
    echo -e "\n******** OPT USERID $user_id ************\n"

    egrep ",OPTSTK,|,OPTIDX," $TRADE_EXPORT_FILE | awk -F, -v usr=${user_id} '{ if($16==usr){ if($13=="B"){print $5,$6,$7,$10,$13,$8,$9} else{ print $5,$6,$7,-1*$10,$13,$8,$9}}}' | awk -v eno1=${curr_exp_no} -v e1=${curr_exp_date} -v e2=${next_exp_date} -v eno2=${next_exp_no} -v e3=${far_exp_date} -v eno3=${far_exp_no} -v eno4=${curr_week_exp_no} -v e4=${curr_week_exp_date} '{if($3==e1){f = eno1}else if($3==en2){f = eno2} else if($3==e3){f=eno3} else if($3==e4){f=eno4} printf "NSE_%s_%s_%s_%s %d %s\n",$2,$7,f,$6,$4,$5}' >>/tmp/gen_opt_pos_exec_

    echo -e "\t~~~~ LAST TRADE ~~~~"
    egrep ",OPTSTK,|,OPTIDX," $TRADE_EXPORT_FILE  | awk -F, -v usr=${user_id} '{ if($16==usr){ if($13=="B"){print $5,$6,$7,$10,$13,$8,$9,$4,$11,$12} else{ print $5,$6,$7,-1*$10,$13,$8,$9,$4,$11,$12}}}' | awk -v eno1=${curr_exp_no} -v e1=${curr_exp_date} -v e2=${next_exp_date} -v eno2=${next_exp_no} -v e3=${far_exp_date} -v eno3=${far_exp_no} -v eno4=${curr_week_exp_no} -v e4=${curr_week_exp_date} '{if($3==e1){f = eno1}else if($3==en2){f = eno2} else if($3==e3){f=eno3} else if($3==e4){f=eno4} printf "%s NSE_%s_%s_%s_%s %d %f %s\n",$8,$2,$7,f,$6,$4,$9,$5}' | sort -rk1| head -2 #"%s NSE_%s%s %d %f %s\n",$6,$2,f,$4,$7,$5}' | head -1
    echo -e "\t~~~~~~~~~~~~~~~~~~~~"
    echo -e "\n***********************************\n"
  done

  cat /tmp/gen_opt_pos_exec_ | awk '{pos[$1]+=$2} END {for(i in pos){ if(pos[i]!=0)  print i,-pos[i]}}' >${OPT_POS_EXEC_FILE} 

  #echo "awk 'NR==FNR { fo_pos[$NF]=$(NF-1); next } { if (fo_pos[$(NF-1)]) {print fo_pos[$(NF-1)],$(NF-1),$NF} else {print "INVALID",$(NF-1),$NF}}' $datasource_exchsymbol_file ${OPT_POS_EXEC_FILE} > /tmp/OPT_exchsymbol_${DATE}"
  awk 'NR==FNR { fo_pos[$NF]=$(NF-1); next } { if (fo_pos[$(NF-1)]) {print fo_pos[$(NF-1)],$(NF-1),$NF} else {print "INVALID",$(NF-1),$NF}}' $datasource_exchsymbol_file ${OPT_POS_EXEC_FILE} > /tmp/OPT_exchsymbol_position_${DATE}
 
  grep -v "INVALID" /tmp/OPT_exchsymbol_position_${DATE} | awk '{print $1}' > /tmp/OPT_exchsymbol_${DATE}

  true > /tmp/OPT_exchsymbol_shortcode_${DATE}

  LD_PRELOAD=/home/dvctrader/important/libcrypto.so.1.1 /home/pengine/prod/live_execs/get_shortcode_from_ds ${DATE} FO /tmp/OPT_exchsymbol_${DATE} /tmp/OPT_exchsymbol_shortcode_${DATE} 

  awk 'NR==FNR { if($NF != "INVALID") fo_pos[$(NF-1)]=$NF; next } { if (fo_pos[$(NF-2)]) print fo_pos[$(NF-2)],$NF}' /tmp/OPT_exchsymbol_shortcode_${DATE} /tmp/OPT_exchsymbol_position_${DATE} >${OPT_POS_EXEC_FILE}

  cat ${OPT_POS_EXEC_FILE} >> ${POS_EXEC_FILE}
}

adjust_addts(){
  #get lot size for fo products
  prod_to_lotsize='/tmp/prod_to_lotsize'
  trade_info_dir='/spare/local/tradeinfo/NSE_Files/'
  lotsize_file=${trade_info_dir}'/Lotsizes/fo_mktlots_'${DATE}'.csv'
  cat ${lotsize_file} | grep -v SYMBOL \
      | awk -F "," '{gsub(" ","",$2);print $2"_0",$3,"\n"$2"_1",$4"\n"$2"_2",$5}' > ${prod_to_lotsize}
  
  declare -A prod_to_lotsize_map
  
  while read -r line;
  do
    prod=`echo ${line} | awk '{print $1}'`;
    lotsize=`echo ${line} | awk '{print $2}'`;
    prod_to_lotsize_map[$prod]=${lotsize}
  done  < ${prod_to_lotsize}

  echo "ADJUST ADDTS:"
  #/home/pengine/prod/live_scripts/adjust_addts.sh
  echo "STRAT ADJUST ADDTS DONE"
  adjust_addts_limits="/tmp/cumulative_limits"
  comb_addts_limits="/tmp/cob_addts_limits_${DATE}"
  comb_addts_file="/tmp/comb_addts_file_${DATE}"
  POS_EXEC_FILE_FOR_ADDTS="/tmp/pos_exec_file_for_addts_${DATE}"
  >${POS_EXEC_FILE_FOR_ADDTS}
  > ${comb_addts_file}

  
  if [ $TYPE == "CM" ]; then
    cp ${POS_EXEC_FILE} ${POS_EXEC_FILE_FOR_ADDTS}
  elif [ $TYPE == "FO" ]; then
    
    #convert fo limits to lot
    if [ -f ${POS_EXEC_FILE} ];
    then
      while read -r line;
      do
        sym=`echo ${line} | awk '{print $1}'`;
        limit=`echo ${line} | awk 'function abs(pos) {return (pos < 0 ? -pos : pos)} {print abs($2)}'`;
        sym_type=`echo $sym | cut -d '_' -f3`
        basename_exp_num=`echo $sym | cut -d '_' -f2`"_"${sym_type: -1}
        
        [ -z ${prod_to_lotsize_map[$basename_exp_num]} ] && continue;
        [ -z ${limit} ] && continue;
        lot_sz=${prod_to_lotsize_map[$basename_exp_num]}
        limit_new=$(( (${limit} + ${lot_sz} - 1 )/ ${lot_sz} ));
        echo "SYM: $sym $limit $sym_type $basename_exp_num $lot_sz $limit_new"
        echo ${sym}" "${limit_new} >> ${POS_EXEC_FILE_FOR_ADDTS}
      done < ${POS_EXEC_FILE}
    fi
  fi


  awk 'function abs(pos) {return (pos < 0 ? -pos : pos)} {print $1,abs($2)}' ${POS_EXEC_FILE_FOR_ADDTS} > ${addts_value_file}
  awk '(NR==FNR) { pos[$1]=$2; next } { print $1,pos[$1]+$2}' ${addts_value_file} ${adjust_addts_limits} > ${comb_addts_limits}
  echo "COMB ADJUSTMENT DONE"

  while IFS= read -r line
  do
    symbol=`echo $line | awk '{print $1}'`
    limit=`echo $line | awk '{print $2}'`;
    ors_current_limit=`grep -w "$symbol" /home/pengine/prod/live_configs/${host_name}_addts.cfg | tail -n1 | awk '{print $5}'`
    if [ $limit -eq 0 ];
    then
       continue;
    elif [ -z $ors_current_limit  ] || [ $ors_current_limit -lt $limit ];
    then
      echo "NSE ${profile} ADDTRADINGSYMBOL \"${symbol}\" ${limit} ${limit} ${limit} $(( 2 * limit))" >> ${comb_addts_file}
    fi
  done < ${comb_addts_limits}

  ORS_DIR=`ps aux | grep cme | grep -v grep | awk '{print $15}'`
  ORS_FILE="${ORS_DIR}/log.${DATE}"
  echo "FILE: $ORS_FILE"
  LAST_APPEND_LINE=`ssh dvcinfra@${SERVER_IP} "wc -l $ORS_FILE | cut -d' ' -f1"`
  echo "LAST APPEND MODE: $LAST_APPEND_LINE"

  if [ -f ${comb_addts_file} ];
  then
    echo "DO COMB ADDTS"
    echo "#/home/pengine/prod/live_scripts/ADDTRADINGSYMBOL.sh ${comb_addts_file}"
    /home/pengine/prod/live_scripts/ADDTRADINGSYMBOL.sh ${comb_addts_file}
  fi

  ERROR_PROD_FILE="/tmp/error_prod_file"
  ADDTS_ERROR_PROD_FILE="/tmp/addts_error_prod_file"
  #filter error products
  ssh dvcinfra@${SERVER_IP} "grep -an "Resetting" $ORS_FILE | awk -v line_no="$LAST_APPEND_LINE" -F':' '{if(\$2>line_no) print \$NF}' | awk -F'.' '{print \$1}' | sort | uniq | awk '{print \$NF,\"RESETTING\"}'" >$ERROR_PROD_FILE
  ssh dvcinfra@${SERVER_IP} "grep -an \"MARGIN NOT PROVIDED\" $ORS_FILE | egrep "ControlThread.*ERROR" | awk -v line_no="$LAST_APPEND_LINE" -F':' '{if(\$1>line_no) print \$NF}' | sort | uniq | awk '{print \$1,\"MARGIN_NOT_PROVIDED\"}'" >>$ERROR_PROD_FILE
  ssh dvcinfra@${SERVER_IP} "grep -an \"UNDER BAN\" $ORS_FILE | egrep "ControlThread.*ERROR" | awk -v line_no="$LAST_APPEND_LINE" -F':' '{if(\$1>line_no) print \$(NF-1)}' | sort | uniq | awk '{print \$1,\"UNDER_BAN\"}'" >>$ERROR_PROD_FILE
  echo "ERROR PROD FILE: $ERROR_PROD_FILE"
 
  #filter addts products
  awk '(NR==FNR) { prod[$1]=$2; next } { if ($1 in prod) { print $1,prod[$1],$2}}' ${POS_EXEC_FILE} $ERROR_PROD_FILE | sort -u  > $ADDTS_ERROR_PROD_FILE
  echo "ADDTS ERROR PROD FILE: $ADDTS_ERROR_PROD_FILE"


: '

  margin_file="/home/pengine/prod/live_configs/common_initial_margin_file.txt"
  securitymargin_file="/spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles/security_margin_${DATE}.txt"
  under_ban_file="/spare/local/tradeinfo/NSE_Files/SecuritiesUnderBan/fo_secban_${DATE}.csv"
  #handling addts of products
  while IFS=' ' read -r prod_code position error
  do
      if [ "$error" == "RESETTING" ]; then
        if [ `grep -w $prod_code $margin_file | head -1 | wc -l` -ge "1" ]; then
          max_pos=`expr 3 \* $(grep -w $prod_code $margin_file | head -1 | awk "function max(a,b) { return a>b ? a:b } {print max(max($2,$3),$4)}")`
          replace_margin=`grep -w $prod_code $margin_file | head -1`
          replace_margin_with="$prod_code $max_pos $max_pos $max_pos"
          replace_margin_with=`sed "s/&/\\\&/g" <<< $replace_margin_with`
          sed "s/$replace_margin/$replace_margin_with/g" $margin_file > /tmp/temp_margin_file
          cat /tmp/temp_margin_file > $margin_file
          echo "RESETTING $replace_margin : $replace_margin_with"

        else
          append_line=`grep -n "NSE_" $margin_file | tail -1 | cut -d":" -f1`
          sed "$append_line a $prod_code 300 300 300" $margin_file > /tmp/temp_margin_file
          cat /tmp/temp_margin_file > $margin_file
          echo "RESETTING $append_line $prod_code"
        fi
        /home/pengine/prod/live_scripts/ors_control.pl NSE $PROFILER RELOADMARGINFILE
       
       elif [ "$error" == "MARGIN_NOT_PROVIDED" ]; then
         prod=`echo $prod_code $securitymargin_file | cut -d_ -f1-2`
         margin=`grep $prod $securitymargin_file | head -1 | cut -d' ' -f2`
         sc_margin="$prod_code $margin"

         if [ `grep -w $prod_code $securitymargin_file | wc -l` == "0" ]; then
           echo ${sc_margin} >> $securitymargin_file
           echo "MARGIN_NOT_PROVIDED ${sc_margin}"
         fi
         /home/pengine/prod/live_scripts/ors_control.pl NSE $PROFILER RELOADSECURITYMARGINFILE

       elif [ "$error" == "UNDER_BAN" ] ; then
         prod_name=`echo $prod_code | cut -d_ -f2`
         sed -i "/$prod_name/d" $under_ban_file
         echo "UNDER_BAN $prod_name"
         /home/pengine/prod/live_scripts/ors_control.pl NSE $PROFILER RELOADMARGINFILE
       fi
  done < $ADDTS_ERROR_PROD_FILE

  if [ -f ${comb_addts_file} ];
  then
    echo "DO COMB ADDTS AGAIN"
    echo "#/home/pengine/prod/live_scripts/ADDTRADINGSYMBOL.sh ${comb_addts_file}"
    #/home/pengine/prod/live_scripts/ADDTRADINGSYMBOL.sh ${comb_addts_file}
  fi
'
}

get_user_file() {
  USERLIST_FILE="/tmp/user_id_${DATE}"
  >${USERLIST_FILE}
  if [ "$USERLIST_FILE_DET" = "IND14" ] || [ "$USERLIST_FILE_DET" = "IND15" ] || [ "$USERLIST_FILE_DET" = "IND16" ] || [ "$USERLIST_FILE_DET" = "IND17" ] || [ "$USERLIST_FILE_DET" = "IND18" ] || [ "$USERLIST_FILE_DET" = "IND19" ] || [ "$USERLIST_FILE_DET" = "IND20" ] || [ "$USERLIST_FILE_DET" = "IND22" ] || [ "$USERLIST_FILE_DET" = "IND23" ]; then   
    serverid=`echo ${USERLIST_FILE_DET} | awk -F"IND" '{print $2}'`
    hst_nm="sdv-ind-srv${serverid}"
    prof="${server_to_profile_[${hst_nm}]}"
    echo "$prof : $serverid"
    ORS_CONFIG="/home/pengine/prod/live_configs/common_${prof}_ors.cfg"
    ors_config_file="/tmp/ors_conf"
    req_ip="${server_to_ip_map[${hst_nm}]}"
    scp dvcinfra@${req_ip}:${ORS_CONFIG} ${ors_config_file}
    for i in `grep Multisessions ${ors_config_file} | awk '{print $2}' | awk -F, '{for(i=1;i<=NF;i++) print $(i)}'`; do grep "UserName-${i}" ${ors_config_file} | awk '{print $2}' >>${USERLIST_FILE}; done
  else
    USERLIST_FILE="${USERLIST_FILE_DET}"
  fi
}

init (){
  [ $# -eq 5 ] || print_msg_and_exit "USAGE: <SCRIPT> <DATE> <TYPE:FO/CM> <TRADE_EXP_FILE> <USERLIST_FILE or srv_name[IND14]> <ADDS-FLAG [0/1]>"

  declare -A server_to_interface_map
  server_to_interface_map=( ["sdv-ind-srv14"]="enp1s0f1np1" \
                          ["sdv-ind-srv15"]="enp101s0f1" \
                          ["sdv-ind-srv16"]="enp1s0f1" \
                          ["sdv-ind-srv17"]="enp101s0f1" \
                          ["sdv-ind-srv18"]="enp101s0f1" \
                          ["sdv-ind-srv19"]="enp101s0f1" \
                          ["sdv-ind-srv21"]="enp1s0f1np1" \
                          ["sdv-ind-srv22"]="enp161s0f1" \
                          ["sdv-ind-srv23"]="enp1s0f1" \
                          ["sdv-ind-srv20"]="enp1s0f1" )

  declare -A server_to_profile_
  server_to_profile_=( ["sdv-ind-srv14"]="MSFO7" \
                     ["sdv-ind-srv15"]="MSFO4" \
                     ["sdv-ind-srv16"]="MSEQ2" \
                     ["sdv-ind-srv17"]="MSEQ3" \
                     ["sdv-ind-srv18"]="MSEQ4" \
                     ["sdv-ind-srv21"]="MSEQ7" \
                     ["sdv-ind-srv22"]="MSFO8" \
                     ["sdv-ind-srv23"]="MSEQ6" \
                     ["sdv-ind-srv19"]="MSFO5" \
                     ["sdv-ind-srv20"]="MSFO6" )

  declare -A server_to_ip_map
  server_to_ip_map=( ["sdv-ind-srv14"]="10.23.227.64" \
                   ["sdv-ind-srv15"]="10.23.227.65" \
                   ["sdv-ind-srv16"]="10.23.227.81" \
                   ["sdv-ind-srv17"]="10.23.227.82" \
                   ["sdv-ind-srv18"]="10.23.227.83" \
                   ["sdv-ind-srv19"]="10.23.227.69" \
                   ["sdv-ind-srv21"]="10.23.227.66" \
                   ["sdv-ind-srv22"]="10.23.227.71" \
                   ["sdv-ind-srv23"]="10.23.227.72" \
                   ["sdv-ind-srv20"]="10.23.227.84" )



  DATE=`date +%Y%m%d`
  DATE=$1
  >/tmp/gen_pos_exec_
  TYPE=$2
  POSTMAN_FILE=$3
  TRADE_EXPORT_FILE="/tmp/trade_export_${DATE}_${TYPE}"
  USERLIST_FILE_DET=$4
  USERLIST_FILE=$4
  ADDTS_FLAG=$5
 
  echo "/home/pengine/prod/live_execs/get_trade_export_file ${POSTMAN_FILE} ${TYPE} | grep -v ERROR  >${TRADE_EXPORT_FILE} "
  /home/pengine/prod/live_execs/get_trade_export_file ${POSTMAN_FILE} ${TYPE} | grep -v ERROR  >${TRADE_EXPORT_FILE} 

  POS_EXEC="/home/dvctrader/ATHENA/pos_exec_sanitize_and_strike_changes_20200723"
  addts_value_file="/tmp/addts_value_${DATE}"
  >${addts_value_file} 
  INTERFACE=${server_to_interface_map[`hostname`]}
  PROFILER=${server_to_profile_[`hostname`]}
  SERVER_IP=${server_to_ip_map[`hostname`]}
  host_name=`hostname`
  profile=`tail -n1 /home/pengine/prod/live_configs/${host_name}_addts.cfg | awk '{print $2}'`

  echo -e "PROFILER: $PROFILER\nSERVER_IP: ${SERVER_IP}\nPROFILER: ${profile}\n"
  tradeinfo_dir='/spare/local/tradeinfo/NSE_Files/'
  datasource_exchsymbol_file="/spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt"
  POS_EXEC_FILE="/tmp/pos_exec_file_${DATE}"
  FUT_POS_EXEC_FILE="/tmp/fut_pos_exec_file_${DATE}"
  OPT_POS_EXEC_FILE="/tmp/opt_pos_exec_file_${DATE}"

  get_user_file $*;

  case $TYPE in
    CM)
      compute_cm_pos_file $*
      ;;
    FO)
      compute_fo_pos_file $*
      ;;
    *)
      print_msg_and_exit "USAGE: <SCRIPT> <TYPE:FO/CM> <TRADE_EXP_FILE> <USERLIST_FILE or SRV_NAME[IND14]> <ADTS_FLAG[0/1]>";
  esac

  echo "export ZF_ATTR=interface=${INTERFACE}; LD_PRELOAD=/home/dvctrader/important/libcrypto.so.1.1 ${POS_EXEC} ${POS_EXEC_FILE} ${DATE} START_TIME(IST_915) 600000 STRAT_ID 1000000 &"

  if [ $ADDTS_FLAG == "1" ]; then
    #adjust_addts $*  
    awk -v pf=${profile} 'function abs(pos) {return (pos < 0 ? -pos : pos)} {print "NSE "pf" ADDTRADINGSYMBOL \""$1"\" "abs($2),abs($2),abs($2),abs($2),2*abs($2)}' ${POS_EXEC_FILE} > ${addts_value_file}
    echo "DOING ADDTS"
    echo "/home/pengine/prod/live_scripts/ADDTRADINGSYMBOL.sh ${addts_value_file}"
    /home/pengine/prod/live_scripts/ADDTRADINGSYMBOL.sh ${addts_value_file}
    echo "ADDTS DONE"
  fi
}

init $*
