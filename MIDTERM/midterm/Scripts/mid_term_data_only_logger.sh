#!/bin/bash

USAGE1="$0 CMD[START/STOP] MODE[LOGGER/SERVER] QID"

if [ $# -ne 3 ] ;
then
    echo $USAGE1;
    exit;
fi

LOGGING_SERVER_EXEC=/home/pengine/prod/live_execs/mid_term_data_server
#LOGGING_SERVER_EXEC=/home/pengine/mid_term_data_server
CONFIG_FILE=/spare/local/files/NSE/MidTermLogs/products2_logger.txt

CMD=$1;
MODE=$2; 
QID=$3
PIDDIR=/spare/local/logs/EXEC_PID_DIR ;

PIDFILE=$PIDDIR/"MIDTERM_DATA_SERVER_"$MODE"_PIDfile.txt"
COUTCERRFILE=/spare/local/logs/alllogs/mid_term_data_server_"$MODE".ceoutcerr ;
AFFIN_EXEC="/home/pengine/prod/live_execs/cpu_affinity_mgr"
if [ $PWD != $HOME ] ; then cd $HOME ; fi

case $CMD in
    start|START)
	if [ -f $PIDFILE ] ;
	then
        # print error & exit
            
	else
        # Appending the mds logger pid to the same file --


            #$LOGGING_SERVER_EXEC >/dev/null 2>&1  &
#            $LOGGING_SERVER_EXEC --products_file $CONFIG_FILE --data_process_mode $MODE >COUTCERRFILE 2>COUTCERRFILE & 
            $LOGGING_SERVER_EXEC --products_file $CONFIG_FILE --data_process_mode $MODE --qid $QID >$COUTCERRFILE 2>$COUTCERRFILE & 
            MDSLOGGERPID=$!
            echo $MDSLOGGERPID > $PIDFILE
            AF_FLAG_FILE="/home/pengine/prod/live_configs/which_affinity_flag_should_i_use_for_"`hostname` ;
            AF_FLAG=`cat $AF_FLAG_FILE` ;
            
            [ "$AF_FLAG" != "AF" ] || $AFFIN_EXEC ASSIGN $MDSLOGGERPID "mid_term_data_server_S" >>$COUTCERRFILE 2>&1

        fi

        MDSLOGGERPID=$!
        echo $MDSLOGGERPID > $PIDFILE
	;;

    stop|STOP)
	if [ -f $PIDFILE ] ;
	then
            # Stop the mds_logger --
	    MDSLOGGERPID=`tail -1 $PIDFILE`
	    kill -2 $MDSLOGGERPID
	    sleep 1;

	    running_proc_string=`ps -p $MDSLOGGERPID -o comm=`;
	    if [ $running_proc_string ] ; then
		echo "patience ... ";
		sleep 4 ;

		running_proc_string=`ps -p $MDSLOGGERPID -o comm=`;
		if [ $running_proc_string ] ; then
		# by now if still runing SIGINT did not work, sending SIGKILL
		    echo "sending SIGKILL" ;
		    kill -9 $MDSLOGGERPID
		fi
	    fi

	    rm -f $PIDFILE
	else
	    # print error & exit
	    echo "Cannot stop an instance of $LOGGING_SERVER_EXEC, PID FILE DOESN'T EXISTS.. $PIDFILE" ; 
	fi
	;;


    *)
	echo CMD $CMD not expected
	;;
esac

