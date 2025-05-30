#!/bin/bash

### ABOUT
### Runs rsync, retrying on errors up to a maximum number of tries.
### Simply edit the rsync line in the script to whatever parameters you need.

# Trap interrupts and exit instead of continuing the loop
function log_err(){
# echo $1
 LOGFILE="`date +%Y-%m-%d`_rsync_tmpFile"
 case $1 in
    10|23|30|35)
      echo "`date`: rsync finished with a network related error: $1"
      ;;
    0)
      echo "`date`: rsync finished without error"
      echo "`date`: Exact no files matched on server" $2
      ;;

    *)  # all other cases
      echo "`date`: rsync finished with an unexpected error: $1"
      ;;
  esac >> "$LOGFILE"
}

function chk_fileno_ssh(){
 #echo "file chck" $1 " --" $2
 #`ssh -q $1 'ls -al "'$2'" | wc -l'`
 co=$(ssh -q $1 'ls  '$2' | wc -l')
 echo $co
}


function rrsynch(){ 
if [ -z "$3" ]; then 
   echo usage: $0 "source user@host path_on_remote"
   return
 fi

trap "echo Exited!; exit;" SIGINT SIGTERM



MAX_RETRIES=50
i=0

# Set the initial return value to failure
false
ret=12
while [ $ret -ne 0 -a $i -lt $MAX_RETRIES ]
do
##This might have a concurrency issue. It's not safe to retry wth --partial specified alone. Previous failed rsync processes on the target can write a short version of the file over ##a completed one. Add the --delay-updates option, this will write to a temporary file and only copy complete files to the actual file path, rendering stray processes inert.
 i=$(($i+1))
 rsync -avz --progress --partial --delay-updates  $1 $2:$3
#logging the ret code
 ret=$?
 echo "ret code" $ret
 log_err $ret
 log_err 0 "params"$1 $2:$3
 co1=$(ls  $1 | wc -l)
 co2=`chk_fileno_ssh $2 $3`
 echo "here..."$co2 $1 $2:$3
 if [[ ( $co1 == $co2 && $ret == 0 ) ]]; then
 log_err 0 "No of files copied '$co2'"
 break 
 fi

done


if [ $i -eq $MAX_RETRIES ]
then
  echo "Hit maximum number of retries, giving up."
fi

}

#run ;
