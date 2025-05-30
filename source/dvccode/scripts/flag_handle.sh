#!/bin/bash

FLAG_FILE=/home/pengine/prod/live_configs/flags_setting.txt

showFlags() {
  [ -f $FLAG_FILE ] || printHelpAndExit "FILE $FLAG_FILE NOT PRESENT ON THIS SERVER";
  cat $FLAG_FILE;
  exit;
}

printHelpAndExit(){
  echo -e $*;
  exit;
}

executeSetFlag(){

  macro=$2;
  strat_id=$3;

  if [ `grep -w $macro $FLAG_FILE | wc -l` -eq 1 ] ; then

     flag_value=`grep -w $macro $FLAG_FILE | awk '{print $3}'`;
 
     if [ "$strat_id" == "ALL" ] ; then 
        for sid in `ps -ef |grep trade_engine_live | grep -v grep  | awk -F"IST_" '{print $3}' | awk '{print $2}'` ; do
           echo "Executing cmd... /home/pengine/prod/live_execs/user_msg --traderid $sid --setunitsize 1 --shortcode $flag_value"; 
           /home/pengine/prod/live_execs/user_msg --traderid $sid --setunitsize 1 --shortcode $flag_value;
        done 
     else 
       echo "Executing cmd... /home/pengine/prod/live_execs/user_msg --traderid $strat_id --setunitsize 1 --shortcode $flag_value";
       /home/pengine/prod/live_execs/user_msg --traderid $strat_id --setunitsize 1 --shortcode $flag_value;
     fi
  else
     printHelpAndExit "PLEASE CHECK GIVEN MACRO $macro, EITHER NOT PRESENT OR MULTIPLE ENTRIES PRESENT IN THE FILE $FLAG_FILE";  
  fi

}

init() {

  if [ $# -eq 0 ] ; then
    showFlags;
  elif [ $# -eq 1 ] && [ "$1" == "help" ] ; then 
    printHelpAndExit "USAGE: <SCRIPT> <set/help> <FLAG_MACRO> <ALL/strat_id> \nEXAMPLE: showflags set ORSRISK_STATUS_SET 123121" 
  elif [ $# -eq 3 ] && [ "$1" == "set" ] ; then 
    executeSetFlag $*;
  else
    printHelpAndExit "USAGE: <SCRIPT> <set/help> <FLAG_MACRO> <ALL/strat_id> \nEXAMPLE: showflags set ORSRISK_STATUS_SET 123121"
  fi
}

init $*
