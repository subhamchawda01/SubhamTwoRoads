#!/bin/bash
#args=("$@")

USAGE=$1
STRAT=$2
PIDDIR=/spare/local/logs/EXEC_PID_DIR 
PIDFILE=$PIDDIR/$STRAT"Reject_Alert_PIDfile.txt"

if [ "$USAGE" ==  "STOP" ];
then
	if [ -f $PIDFILE ];
	then
		PID=`tail -1 $PIDFILE`
		echo $PID
		kill $PID
		rm $PIDFILE
	else
		echo "You cannot stop what you didn't start!!"
	fi

elif [ "$USAGE" == "START" ];
then
	if [ -f $PIDFILE ] ;
	then
		# print error & exit
        	echo "Please stop the exec before starting again" ;
	else
		PID=$$
		echo $PID > $PIDFILE
        last_reject=0
        last_np=0
        last_lis_not_added=0
        date_="$(date +'%Y%m%d')"
        while true
        do
            words=( $(grep Rej "/spare/local/logs/alllogs/MediumTerm/"$STRAT"_execlogic_dbglog."$date_ | tail -1) )
            if [[ "$last_reject" != "${words[3]}" && "${#words[@]}" != "0" ]] 
            then 
                line="ALERT: Order Rejected for instrument ${words[19]} from $STRAT strategy due to reason ${words[13]}!! "
                echo $line
                /home/pengine/prod/live_execs/send_slack_notification nsemed DATA "$line"
                sleep 150
                words=( $(grep Rej "/spare/local/logs/alllogs/MediumTerm/"$STRAT"_execlogic_dbglog."$date_ | tail -1) )
		last_reject=${words[3]}
            fi
            sleep 30

            words=$(grep ALERT "/spare/local/logs/alllogs/MediumTerm/"$STRAT"_execlogic_dbglog."$date_ | tail -1)
            if [[ "$last_np" != "$words" && "$words" != "0" ]]
            then
                line="ALERT: Time check failing, ordersfile needs to be fixed!! "
                echo $line
                /home/pengine/prod/live_execs/send_slack_notification nsemed DATA "$words"
                sleep 150
                words=$(grep ALERT "/spare/local/logs/alllogs/MediumTerm/"$STRAT"_execlogic_dbglog."$date_ | tail -1)
                last_np=$words
            fi
            sleep 30

            words=( $(grep Listener "/spare/local/logs/alllogs/MediumTerm/"$STRAT"_execlogic_dbglog."$date_ | tail -1) )
            if [[ "$last_lis_not_added" != "${words[@]}" && "${#words[@]}" != "0" ]]
            then
                line="ALERT: "$words
                echo $line
                /home/pengine/prod/live_execs/send_slack_notification nsemed DATA "$line"
                sleep 150
                words=( $(grep Listener "/spare/local/logs/alllogs/MediumTerm/"$STRAT"_execlogic_dbglog."$date_ | tail -1) )
                last_lis_not_added=${words[@]}
            fi
            sleep 30
        done   
	fi
else
	echo "Enter valid command to start or stop the exec"
fi
