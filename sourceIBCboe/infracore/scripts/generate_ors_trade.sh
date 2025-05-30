#!/bin/bash

print_msg_and_exit (){
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


compute_fo_ors_trade(){


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

  #FUT
  >/tmp/comb_ors_trade
  for user_id in `cat $USERLIST_FILE`;
  do
    echo $user_id
    awk -F, -v usr=${user_id} '{ gsub(/ /,"",$10); if($16==usr){ if($13=="B"){print $5","$6","$7","$10","$13","$8","$9","$11",0,"$4} else{ print $5","$6","$7","$10","$13","$8","$9","$11",1,"$4}}}' ${TRADE_EXPORT_FILE} | awk -F,  -v eno1=${curr_exp_no} -v e1=${curr_exp_date} -v e2=${next_exp_date} -v eno2=${next_exp_no} -v e3=${far_exp_date} -v eno3=${far_exp_no} -v eno4=${curr_week_exp_no} -v e4=${curr_week_exp_date} '{if($1 == "OPTSTK" || $1 == "OPTIDX"){if($3==e1){f = eno1}else if($3==e2){f = eno2} else if($3==e3){f=eno3} else if($3==e4){f=eno4} printf "NSE_%s_%s_%s_%s %d %.2f %d %s\n",$2,$7,f,$6,$4,$8,$9,$10 }else if($1 == "FUTSTK" || $1 == "FUTIDX"){if($3==e1){f = "_FUT0"}else if($3==e2){f = "_FUT1"} else if($3==e3){f="_FUT2"} printf "NSE_%s%s %d %.2f %d %s\n",$2,f,$4,$8,$9,$10}}' >>/tmp/comb_ors_trade
  done
  echo "DONE"
  
#  cat /tmp/gen_pos_exec_ | awk '{pos[$1]+=$2} END {for(i in pos){ if(pos[i]!=0)  print i,-pos[i]}}'i >${FUT_POS_EXEC_FILE}
  grep _FUT /tmp/comb_ors_trade > /tmp/comb_fut_ors_trade
  grep _FUT /tmp/comb_ors_trade | awk '{print $1}' | sort -u  > /tmp/sc.txt
  LD_PRELOAD=~/important/libcrypto.so.1.1 /home/pengine/prod/live_execs/get_exchange_symbol_file /tmp/sc.txt ${DATE} | sort -u >/tmp/fut_shc_exch
  
  awk 'NR==FNR { exch_sym[$(NF-1)]=$(NF); next } { if (exch_sym[$1]) {print exch_sym[$1],$4,$2,$3" 1 1 1 1"} else {print "INVALID",$1,$4,$2,$3}}' /tmp/fut_shc_exch /tmp/comb_fut_ors_trade >/tmp/fut_ors_trade

  #OPT
  grep -v _FUT /tmp/comb_ors_trade > /tmp/comb_opt_ors_trade
  grep -v _FUT /tmp/comb_opt_ors_trade | awk '{print $1}' | sort -u > /tmp/opt_ds
  true > /tmp/OPT_exchsymbol_shortcode_${DATE}  

  awk 'NR==FNR { fo_pos[$NF]=$(NF-1); next } { if (fo_pos[$1]) {print fo_pos[$1],$4,$2,$3" 1 1 1 1"} else {print "INVALID",$1,$4,$2,$3}}' $datasource_exchsymbol_file /tmp/comb_opt_ors_trade >/tmp/opt_ors_trade

  cat /tmp/fut_ors_trade | tr ' ' '\001' >${ORS_TRADE_FILE}
  cat /tmp/opt_ors_trade | tr ' ' '\001' >>${ORS_TRADE_FILE}
}

compute_cm_ors_trade(){
  >${ORS_TRADE_FILE}
  for user_id in `cat $USERLIST_FILE`;
  do
    echo "userid: $user_id"
    cat ${TRADE_EXPORT_FILE} | grep -v "Code,Remarks," | awk -F"," -v usr=${user_id} '{if($13==usr){if($10 == "B") {print $5" 0 "$7" "$8" 1 1 1 1"} else {print $5" 1 "$7" "$8" 1 1 1 1"}}}' | awk '{print "NSE_"$1" "$2" "$3" "$4" "$5" "$6" "$7" "$8}' | tr ' ' '\001' >>${ORS_TRADE_FILE} 
  done
}

init (){
  [ $# -eq 3 ] || print_msg_and_exit "USAGE: <SCRIPT> <TYPE:FO/CM> <TRADE_EXP_FILE> <USERLIST_FILE>"
  TYPE=$1 
  TRADE_EXPORT_FILE=$2 
  ORS_TRADE_FILE="/tmp/ors_trade_file"
  DATE=`date +%Y%m%d`
  >/tmp/gen_pos_exec_
  TYPE=$1
  NOTIS_API_FILE=$2
  TRADE_EXPORT_FILE="/tmp/trade_export_file_${TYPE}_${DATE}"
  USERLIST_FILE=$3
  host_name=`hostname`

  /home/pengine/prod/live_execs/get_trade_export_file ${NOTIS_API_FILE} ${TYPE} | grep -v ERROR  >${TRADE_EXPORT_FILE}
  profile=`tail -n1 /home/pengine/prod/live_configs/${host_name}_addts.cfg | awk '{print $2}'`

  echo -e "PROFILER: $PROFILER\nSERVER_IP: ${SERVER_IP}\nPROFILER: ${profile}\n"
  tradeinfo_dir='/spare/local/tradeinfo/NSE_Files/'
  datasource_exchsymbol_file="/spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt"
  POS_EXEC_FILE="/tmp/pos_exec_file_${DATE}"
  FUT_POS_EXEC_FILE="/tmp/fut_pos_exec_file_${DATE}"
  OPT_POS_EXEC_FILE="/tmp/opt_pos_exec_file_${DATE}"


  case $TYPE in
    CM)
      compute_cm_ors_trade $*
      ;;
    FO)
      compute_fo_ors_trade $*
      ;;
    *)
      print_msg_and_exit "USAGE: <SCRIPT> <TYPE:FO/CM> <TRADE_EXP_FILE> <USERLIST_FILE>";
  esac

  echo "ORS_TRADE_FILE: ${ORS_TRADE_FILE}"
}

init $*
