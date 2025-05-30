#!/bin/bash

print_msg_and_exit (){
  echo $* ; 
  exit ; 
}

init (){
  SPREAD_EXEC=$HOME/basetrade_install/bin/spread_exec ;
  DATE_EXEC=$HOME/basetrade_install/bin/update_date ; 
}
#Cleanup
for f in /spare/local/MDSlogs/MT_SPRD_EXEC/*_$6; do echo "deleting "$f; rm $f; done

for f in /spare/local/MDSlogs/MT_SPRD_TRADES/*_$6; do echo "deleting "$f; rm $f; done

for f in /spare/local/MDSlogs/MT_SPRD_NAV_SERIES/*_$6; do echo "deleting "$f; rm $f; done

if [ $1 -eq 1 ]; then
  mult='--multiparam';
else
  mult='';
fi  


if [ $2 -eq 1 ]; then
  exlg='--use_exec_logic --trade_start_date 20161001';
else
  exlg='';
fi

run(){

  [ $# -eq 6 ] || print_msg_and_exit "Usage : < script > < multiparam 0/1 > < use exec logic 0/1 > < PARAMFILE > < START_DATE > < END_DATE > < PARAM ID >" ;

  [ $4 -le $5 ] || print_msg_and_exit "Start Date Can Not Be Greater Than End Date" ; 

#For the first date let's run it with save mode 
  if [ $4 -eq $5 ] ; then   #Only running for a day 
    $SPREAD_EXEC --notify_last_event --paramfile $3 --start_date $4 --end_date $5 --save_state --hft_data $mult $exlg;
    echo "$SPREAD_EXEC --notify_last_event --paramfile $3 --start_date $4 --end_date $4 --save_state --hft_data" $mult $exlg;
  else 
    $SPREAD_EXEC --paramfile $3 --start_date $4 --end_date $4 --save_state --hft_data $mult $exlg; 
    echo "$SPREAD_EXEC --paramfile $3 --start_date $4 --end_date $4 --save_state --hft_data" $mult $exlg;
    nextdate=`$DATE_EXEC $4 N W` ;

#Intermediate Runs 
    while [ $nextdate -lt $5 ]; 
    do
      $SPREAD_EXEC --paramfile $3 --start_date $nextdate --end_date $nextdate --load_state --save_state --hft_data $mult $exlg;
      echo "$SPREAD_EXEC --paramfile $3 --start_date $nextdate --end_date $nextdate --load_state --save_state --hft_data" $mult $exlg;
      nextdate=`$DATE_EXEC $nextdate N W` ;
    done 

#Final Run With NotifyLastEvent       
    echo "$SPREAD_EXEC --notify_last_event --paramfile $3 --start_date $nextdate --end_date $nextdate --load_state --save_state --hft_data" $mult $exlg;
    $SPREAD_EXEC --notify_last_event --paramfile $3 --start_date $nextdate --end_date $nextdate --load_state --save_state --hft_data $mult $exlg;
  fi 
}

init ;
run $* ;
