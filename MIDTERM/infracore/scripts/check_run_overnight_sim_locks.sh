#!/bin/bash

USAGE1="$0 $LOCK_DIR" 
EXAMP1="$0 /home/dvctrader/locks"

if [ $# -ne 1 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

LOC_DIR=$1; shift ;

MAIL_FILE=/tmp/call_run_overnight_alert

>$MAIL_FILE ;

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH



#longer 
for lockfiles in `ls $LOC_DIR/call_run_*longer*lock`
do 

    this_shortcode_lockfile_=`echo $lockfiles | awk -F"/" '{print $NF}' | sed 's/call_run_sim_overnight_longer_//g' | awk -F"." '{print $1}'` ; 

    is_running_=`ps -ef | grep "call_run_sim_overnight" | grep "longer" | grep $this_shortcode_lockfile_ | wc -l` ;

    if [ $is_running_ -lt 1 ]
    then 

        echo "Lock File Present : "$lockfiles " Process Not Running For : "$this_shortcode_lockfile_ >> $MAIL_FILE 

    fi 

done 

#non-longer 
for lockfiles in `ls $LOC_DIR/call_run_*lock | grep -v "longer"`
do 

    this_shortcode_lockfile_=`echo $lockfiles | awk -F"/" '{print $NF}' | sed 's/call_run_sim_overnight_//g' | awk -F"." '{print $1}'` ; 

    is_running_=`ps -ef | grep "call_run_sim_overnight" | grep -v "longer" | grep $this_shortcode_lockfile_ | wc -l` ;

    if [ $is_running_ -lt 1 ]
    then 

        echo "Lock File Present : "$lockfiles " Process Not Running For : "$this_shortcode_lockfile_ >> $MAIL_FILE 

    fi 

done 

LINE_COUNT=`wc -l $MAIL_FILE | awk '{print $1}'`

if [ $(($LINE_COUNT)) -gt 0 ]
then

    /bin/mail -s "RunOverNightSim" -r "checkrunovernight@ny11" "nseall@tworoads.co.in" < $MAIL_FILE 

fi

rm -rf $MAIL_FILE ;

