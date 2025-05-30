#!/bin/bash

USAGE1="$0 EXCH SESSION TIMEOUT CMD[START/STOP]";

if [ $# -ne 4 ] ;
then
    echo $USAGE1;
    exit;
fi

EXCH=$1;
SESSION=$2;
TIMEOUT=$3;
CMD=$4;

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

 
DETECTOR_SCRIPT=LiveExec/scripts/exchange_reply_monitor.sh

ORS_LOGS_DIR=/spare/local/ORSlogs ;
PIDDIR=/spare/local/ORSlogs/ExchangeResponse/PID_ORS_DIR ;
PIDFILE=$PIDDIR/$EXCH"_"$SESSION"_"PIDfile.txt ;

FIX_LOGS_COUNT=`ls $ORS_LOGS_DIR/$EXCH/$SESSION/OPTIMUMFIX*messages.log | wc -l`

if [ $FIX_LOGS_COUNT -lt 1 ]
then

   echo "FIX LOG COUNT : " $FIX_LOGS_COUNT
   exit;

fi

FIX_LOG_FILE=$ORS_LOGS_DIR/$EXCH/$SESSION/OPTIMUMFIX*messages.log ;
TEMP_REPLY_DUMP_FILE=$ORS_LOGS_DIR/ExchangeResponse/$EXCH"_"$SESSION"_exchange_response.log"  ;

if [ ! -d $PIDDIR ] ; then mkdir -p $PIDDIR ; fi ;

>$TEMP_REPLY_DUMP_FILE

TAIL_CMD="tail -f $FIX_LOG_FILE" 
#line-buffered so it works with growing file
GRP_CMD="grep --line-buffered >>"


case $CMD in
    start|START)
        if [ -f $PIDFILE ] ;
        then
            # print error & exit
            echo "Cannot start an instance of Inactive Session Detector for $EXCH session $SESSION since $PIDFILE exists"
        else
            # Appending the ors bcast logger pid to the same file -- 
            # create the file
            ls > $PIDFILE
            
            $TAIL_CMD | $GRP_CMD >> $TEMP_REPLY_DUMP_FILE &
            CMDPID=$!

            echo $CMDPID > $PIDFILE

            $DETECTOR_SCRIPT $TEMP_REPLY_DUMP_FILE $EXCH $SESSION $TIMEOUT &
            SCRIPTPID=$!

            echo $SCRIPTPID >> $PIDFILE

        fi
        ;;

    stop|STOP)
        if [ -f $PIDFILE ] ;
        then
            SCRIPTPID=`tail -1 $PIDFILE`
            CMDPID=`head -1 $PIDFILE`
            kill -9 $SCRIPTPID $CMDPID
            sleep 1;

            running_proc_string=`ps -p $SCRIPTPID -o comm=`;
            if [ $running_proc_string ] ; then
                sleep 5 ;

                running_proc_string=`ps -p $SCRIPTPID -o comm=`;
                if [ $running_proc_string ] ; then
                    echo "sending SIGKILL" ;
                    kill -9 $SCRIPTPID
                fi
            fi

            rm -f $PIDFILE
        else
            # print error & exit
            echo "Cannot stop an instance of Inactive Session Detector for $EXCH session $SESSION since $PIDFILE does not exist"
        fi

        ;;

    *)
        echo CMD $CMD not expected
        ;;
esac
