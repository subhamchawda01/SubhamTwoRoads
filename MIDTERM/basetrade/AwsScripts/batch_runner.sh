#!/bin/bash
#IFS=$'\n'
FILE_DIR=$HOME/AWSScheduler
for task in `cat $FILE_DIR/AWSBatchRun.bat | tr ' ' '~'`
  do
    cmd=`echo $task | tr '~' ' '` ;
    #echo $cmd
    $cmd;
#    echo "Running task: $cmd"
#    `$cmd`
  done
