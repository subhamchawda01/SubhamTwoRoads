#!/bin/bash
#This script manages locks for pick_strats_and_install.pl (so that no multiple queries get installed on same UTC day)
#Modes: START=>create running lock, STOP=>remove running lock, COMPLETE=>create done lock
if [ $# -lt 2 ]; then
  echo "USAGE: $0 query-id [START/START_FORCE/STOP/COMPLETE]\n(Checks and creates pick_strats lock for the provided query id)";
  exit 1;
fi

query_id="$1"
mode="$2"
#ssh to ny411 and check/create lock
ssh -o ConnectTimeout=30 dvctrader@10.23.74.51 <<CHECK_AND_CREATE_LOCK
  
  #Check for presence of lock directory
  running_lock_dir=/home/dvctrader/.pickstrats_locks_running
  done_lock_dir=/home/dvctrader/.pickstrats_locks
  if [ ! -d "\$running_lock_dir" ]; then
    mkdir -p "\$running_lock_dir"
  fi
  if [ ! -d "\$done_lock_dir" ]; then
    mkdir -p "\$done_lock_dir"
  fi

  #Individual query lock
  query_lock="\$running_lock_dir/$query_id"
  if [ "$mode" == "COMPLETE" ]; then
      query_lock="\$done_lock_dir/$query_id"
  fi

  #Check if query running/done lock already exists from previous UTC day (if yes, then remove it)
  #If created today, then it means another install is running => show/mail error message
  if [ -d "\$query_lock" ]; then
    current_date="\$(date +%Y%m%d)"
    creation_date="\$(date +%Y%m%d -r \$query_lock)"
    
    if [ "\$current_date" -ne "\$creation_date" ]; then
      #Old lock (from previous day)
      rmdir \$query_lock
    else
      #Duplicate installs
      if [ "$mode" == "STOP" ]; then
        #Remove running lock file
        \`rmdir \$query_lock\`
        exit "$?"
      elif [ "$mode" == "START_FORCE" -o "$mode" == "COMPLETE" ]; then
        #Remove running/done lock forcefully
        \`rmdir \$query_lock\` 
      else
        #One more install is running already
        echo "Error: Lock directory \$query_lock already exists from today (probably trying duplicate installs). You may try again after removing the lock dir \$query_lock if you are sure everything is fine. Exiting!"
        exit 1
      fi
    fi
  fi
  
  if [ "$mode" == "STOP" ]; then
    #do nothing as the lock does not exist
    echo "\$query_lock does not exist"
    exit 1
  fi

  #If START mode, then check if done lock has already been created
  query_done_lock="\$done_lock_dir/$query_id"
  if [ "$mode" == "START" -a -d \$query_done_lock ]; then
    current_date="\$(date +%Y%m%d)"
    creation_date="\$(date +%Y%m%d -r \$query_done_lock)"
            
    if [ "\$current_date" -ne "\$creation_date" ]; then
      #Old lock (from previous day)
      rmdir \$query_done_lock
    else
      #Attempting duplicate installs
      echo "Error: Lock directory \$query_done_lock already exists from today (probably trying duplicate installs). You may try again after removing the lock dir \$query_lock if you are sure everything is fine. Exiting!"
      exit 2
    fi
  fi

  #If exit code of mkdir is non-zero, it failed (useful in case of race conditions)
  if [ \`mkdir \$query_lock\` ]; then
    echo "Unable to create a fresh lock directory \$query_lock (you are either trying duplicate installs or there is a permission issue). Exiting!"
    exit 1;
  fi

CHECK_AND_CREATE_LOCK

#Check the exit code of the previous ssh command
exit "$?"
