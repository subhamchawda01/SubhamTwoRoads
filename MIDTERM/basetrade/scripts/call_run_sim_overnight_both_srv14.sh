#!/bin/bash

if [ -e $HOME/.gcc_profile ] ; 
then 
    source $HOME/.gcc_profile ; 
else
    export NEW_GCC_LIB=/usr/local/lib
    export NEW_GCC_LIB64=/usr/local/lib64
    export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH
fi

export PATH=$PATH:/sbin
#NUMRUNSIMS=`ps -efH | grep call_run_sim_overnight_both.sh | grep -v grep | grep -v /bin/sh | wc -l`;
#NUML1SIMS=`ps -efH | grep call_run_sim_overnight_longer_1.sh | grep -v grep | wc -l`;
#NUML2SIMS=`ps -efH | grep call_run_sim_overnight_longer_2.sh | grep -v grep | wc -l`;
NUMRUNSIMS=`pidof -x run_simulations.pl | wc -w`
#echo $NUMRUNSIMS $NUMRECSIMS $NUML1SIMS $NUML2SIMS
if [ "$NUMRUNSIMS" -lt "13" ] ; then
    NUMRECSIMS=`pidof -x call_run_sim_overnight_recent.sh | wc -w`;
    if [ "$NUMRECSIMS" -lt "10" ] ; then 
	$HOME/basetrade/scripts/call_run_sim_overnight_recent_srv14.sh
    fi
    NUML1SIMS=`pidof -x call_run_sim_overnight_longer_1.sh |wc -w`;
    if [ "$NUML1SIMS" -lt "7" ] ; then 
	$HOME/basetrade/scripts/call_run_sim_overnight_longer_1_srv14.sh
    fi
    NUML2SIMS=`pidof -x call_run_sim_overnight_longer_2.sh | wc -w`;
    if [ "$NUML2SIMS" -lt "7" ] ; then
	$HOME/basetrade/scripts/call_run_sim_overnight_longer_2_srv14.sh
    fi
#    echo "Not enough running $NUMRUNSIMS";
# else
#     echo "Not adding any more runsim instances. $NUMRUNSIMS instances running already.";
else
    NUMSIMSTRATEGY=`pidof sim_strategy | wc -w`
    if [ $NUMSIMSTRATEGY -lt 5 ] # run_sim2 is running but it is not spawning sim_strategy 
    then
	sleep 10; # wait and check once
	NUMSIMSTRATEGY=`pidof sim_strategy | wc -w`
	if [ $NUMSIMSTRATEGY -lt 5 ] # run_sim2 is running but it is not spawning sim_strategy 
	then
	    hn=`hostname`
#	echo "TO: nseall@tworoads.co.in
	    echo "FROM: callrunsimovernight@$hn
TO: nseall@tworoads.co.in
SUBJECT: run_simulation_2.pl seems to have stuck.

Only $NUMSIMSTRATEGY sim_straegy's are running while $NUMRUNSIMS run_simulation_2.pl are running. 
Process Snapshot" > /tmp/mailfl
	    ps -efH | grep dvctrader | grep sim  >> /tmp/mailfl 
	    cat /tmp/mailfl | /usr/sbin/sendmail -it
	fi
    fi
fi
