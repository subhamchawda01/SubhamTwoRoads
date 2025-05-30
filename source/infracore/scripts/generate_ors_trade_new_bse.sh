#!/bin/bash

print_msg_and_exit (){
  echo $* ;
  exit ;
}

compute_fo_ors_trade(){

  echo "NOT YET IMPLEMENTED FOR FO"
}

compute_cm_ors_trade(){
  >${ORS_TRADE_FILE}
  for user_id in `cat $USERLIST_FILE`;
  do
    user_last_4_id=${user_id: -4};
    echo "userid: $user_id  user_last_4_id: $user_last_4_id"
    cat ${TRADE_EXPORT_FILE} | grep -v "Quantity, ScripCode," | awk -F"," -v usr=${user_last_4_id} '{if($1==usr){if($2 == "B") {print $5" 0 "$3" "$6/100" 1 1 1 1"} else {print $5" 1 "$3" "$6/100" 1 1 1 1"}}}' | awk '{print "BSE_"$1" "$2" "$3" "$4" "$5" "$6" "$7" "$8}' | tr ' ' '\001' >>${ORS_TRADE_FILE} 
  done
}


init (){
  [ $# -eq 3 ] || print_msg_and_exit "USAGE: <SCRIPT> <TYPE:FO/CM> <TRADE_EXP_FILE> <USERLIST_FILE>"
  TYPE=$1 
  TRADE_EXPORT_FILE=$2 
  ORS_TRADE_FILE="/tmp/ors_trade_file"
  DATE=`date +%Y%m%d`
  USERLIST_FILE=$3

  POS_EXEC_FILE="/tmp/pos_exec_file_${DATE}"

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
