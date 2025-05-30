#!/bin/bash

USAGE1="$0 FILE EXCH SESSION TIMEOUT";

if [ $# -ne 4 ] ;
then
    echo $USAGE1;
    exit;
fi

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


REPLY_FILE=$1;
EXCH=$2;
SESSION=$3;
TIMEOUT=$4;

while true
do 

  msg_count=`wc -l $REPLY_FILE | awk '{print $1}'`

  if [ $msg_count -lt 1 ]
  then

    echo "No Response From $EXCH $SESSION in $TIMEOUT seconds" |  /bin/mail -s "$HOSTNAME" -r "fixresponse" "nseall@tworoads.co.in"

  fi

  >$REPLY_FILE ;

  sleep $TIMEOUT ;

done
