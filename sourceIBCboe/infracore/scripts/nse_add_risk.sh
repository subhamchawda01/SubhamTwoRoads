#!/bin/bash

print_msg_and_exit () {
    echo $* ;
    exit ;
}

check_and_add_symbol () {
  shc=$1; 

  if [ `grep -w "$shc" $addtsfile | wc -l` -eq 0 ] ; then 
    echo "NSE MSFO ADDTRADINGSYMBOL $shc $maxpos $ordersize $liveorders $worstcase DISABLEHARDCHECKS" >> $addtsfile ;
  else
    mp=`grep -w "$shc" $addtsfile | awk '{print $5}'`;
    os=`grep -w "$shc" $addtsfile | awk '{print $6}'`;

    if [ $mp -lt $maxpos ] || [ $os -lt $ordersize ] ; then 
      grep -v -w "$shc" $addtsfile > $addtsfile".tmp" ;
      echo "NSE MSFO ADDTRADINGSYMBOL $shc $maxpos $ordersize $liveorders $worstcase DISABLEHARDCHECKS" >> $addtsfile".tmp" ; 
      mv $addtsfile".tmp" $addtsfile ;
    fi  

  fi
}

[ $# -ge 2 ] || print_msg_and_exit "Usage : < script > < Underlying > < RiskType - TEST/LOW/MED/HIGH > < SecType - ALL/FUT/OPT > < Action - ADD/REMOVE > " ;

if [ "dvcinfra" != "$USER" ] 
then 
  print_msg_and_exit "Please Run The Script From dvcinfra User..." ;
fi 

underlying=$1; 
risk_type=$2; 
sec_type_=$3;
add_or_remove_=$4;

maxpos=0;
ordersize=0;

if [ "$risk_type" == "TEST" ]; then 
  maxpos=2;
  ordersize=1;
elif [ "$risk_type" == "LOW" ] ; then 
  maxpos=5;
  ordersize=2;
elif [ "$risk_type" == "MED" ] ; then 
  maxpos=10;
  ordersize=3;
elif [ "$risk_type" == "HIGH" ]  ; then 
  maxpos=20;
  ordersize=4;
else   
  print_msg_and_exit "Usage : < script > < Underlying > < RiskType - TEST/LOW/MED/HIGH > < SecType - ALL/FUT/OPT > < Action - ADD/REMOVE > " ;
fi

liveorders=$((maxpos*2));
worstcase=$((maxpos*2));

addtsfile=/home/dvcinfra/LiveExec/config/AddTradingSymbolConfig; 

option_shortcodes_exec="/home/pengine/prod/live_execs/get_option_shortcodes"

yyyymmdd=`date +%Y%m%d`

[ -f $addtsfile -a -s $addtsfile -a -w $addtsfile ] || print_msg_and_exit "addtsfile -> $addtsfile EITHER DOESN'T EXIST OR HAS SIZE ZERO OR NOT WRITABLE" ;


if [[ "$add_or_remove_" == "REMOVE" ]]; then
  if [[ "$underlying" == "C0" || "$underlying" == "P0" ]]; then
    print_msg_and_exit "You are not allowed to do this, please provide correct underlying"
  fi
  #Remove all shortcodes for the underlying
  if [[ "$sec_type_" == "ALL" ]]; then
    cat $addtsfile | grep -v "_"$underlying"_" > /tmp/temp_remove_addts_file
  fi
  #Remove only shortcodes containing calls and puts for that underlying
  if [[ "$sec_type_" == "OPT" ]]; then
    cat $addtsfile | grep -v "_"$underlying"_P" |  grep -v "_"$underlying"_C" > /tmp/temp_remove_addts_file 
  fi
  #Remove only shortcodes containing futures for that underlying
  if [[ "$sec_type_" == "FUT" ]]; then
    cat $addtsfile | grep -v "_"$underlying"_FUT"> /tmp/temp_remove_addts_file
  fi
  mv /tmp/temp_remove_addts_file $addtsfile
  msg=`echo "RiskRemoveRequestFor-> \t $1 \t RiskType-> \t $2 \t for $3 shortcodes"`
  /home/pengine/prod/live_execs/send_slack_notification "nsemed" DATA $msg
  exit 
fi

#Not add futures if only OPT requested
if [[ "$sec_type_" != "OPT" ]]; then
  fut0_shortcode="NSE_"$underlying"_FUT0";
  fut1_shortcode="NSE_"$underlying"_FUT1";
  atm_call="NSE_"$underlying"_C0_A";
  atm_put="NSE_"$underlying"_P0_A";
check_and_add_symbol $fut0_shortcode;
check_and_add_symbol $fut1_shortcode;
fi 

#If only fut requested then add fut and exit
if [[ "$sec_type_" == "FUT" ]]; then
  grep "NSE_"$underlying"_" $addtsfile > /tmp/temp_addts_file ;
  /home/dvcinfra/LiveExec/scripts/ADDTRADINGSYMBOL.sh /tmp/temp_addts_file ;
  msg=`echo "RiskAddRequestFor-> \t $1 \t RiskType-> \t $2 \t for $3 shortcodes"` ;
  /home/pengine/prod/live_execs/send_slack_notification "nsemed" DATA $msg
  exit
fi

#Add Fut as well as options otherwise
for shc in `$option_shortcodes_exec $underlying $yyyymmdd` ; 
do 
check_and_add_symbol $shc
done 

grep "NSE_"$underlying"_" $addtsfile ; 

grep "NSE_"$underlying"_" $addtsfile > /tmp/temp_addts_file ;

#only execute addts for newly added shortcode
/home/dvcinfra/LiveExec/scripts/ADDTRADINGSYMBOL.sh /tmp/temp_addts_file ;

#execute cron addts
#`crontab -l | grep $addtsfile | awk -F"1-5" '{print $2}'`

msg=`echo "RiskAddRequestFor-> \t $1 \t RiskType-> \t $2"` ;

/home/pengine/prod/live_execs/send_slack_notification "nsemed" DATA $msg

