#!/bin/bash
FILE_DIR=$HOME/AWSScheduler
if [ -f $FILE_DIR/lockfile ]
then
  echo "Lock file exists. Can't start process";
  exit 1;
fi
touch $FILE_DIR/lockfile
#$FILE_DIR/gen_new_queue.sh
right_now=`date`
echo "Starting status monitor at: $right_now"
$FILE_DIR/status_monitor.sh
right_now=`date`
echo "Starting scheduler at: $right_now"
$FILE_DIR/AWSScheduler
right_now=`date`
echo "Starting batch_runner at: $right_now"
$FILE_DIR/batch_runner.sh

right_now=`date`
echo "Completed process at: $right_now"
#$FILE_DIR/status_monitor.sh
rm $FILE_DIR/lockfile
