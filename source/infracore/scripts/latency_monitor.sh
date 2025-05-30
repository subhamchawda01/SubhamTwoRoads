#!/bin/bash

USAGE1="$0 PACKETS"
EXAMP1="$0 1000"

if [ $# -ne 1 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

#### AVG VALUES MESURED FROM NY16 after sending 10000 packets to each locations ###
AVG_NY2_CHI=15.324
AVG_NY2_FR2=74.640
AVG_NY2_TOR=26.200
AVG_NY2_BRZ=108.875
#########################################################

PACKETS=$1

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


### IF change is more than a precent send alert ######
HIGH_PERC_ALERT=1.01
LOW_PERC_ALERT=0.99

BRZ_USEC_THRESHOLD=500
#################################################

## Locations to Ping all eth5 except TOR (eth4)##
CHI=10.23.228.51
FR2=10.23.232.51
TOR=10.23.182.51
BRZ=10.23.23.11

MAIL_FILE=/tmp/ping_latency_out.txt
LATENCY_FILE=/tmp/ping_results_hist.txt   # store results in plottable format, append mode only

NY2_CHI_L=0
NY2_FR2_L=0
NY2_TOR_L=0
NY2_BRZ_L=0

touch $MAIL_FILE
>$MAIL_FILE

#### CHI11
NY2_CHI_L=`ping -c$PACKETS -i0.2 -fq $CHI | awk '{print $7}' | tr '/' ' ' | awk '{print $2}'` 

if [ `echo $NY2_CHI_L | wc -c` == 1 ] 
then 
  NY2_CHI_L=-1;
fi

echo $NY2_CHI_L | awk '{ if ( $1 == -1 ) { print "NY-CHI PING ERROR"} else if( $1 > ( '$AVG_NY2_CHI' * '$HIGH_PERC_ALERT' ) ) { print "Higher NY CHI Latency : " $1 " Base Average : " '$AVG_NY2_CHI' } else if( $1 < ( '$AVG_NY2_CHI' * '$LOW_PERC_ALERT' ) ) {print "Lower NY CHI Latency : " $1 " Base Average : " '$AVG_NY2_CHI' } }' >> $MAIL_FILE

#### FR11
NY2_FR2_L=`ping -c$PACKETS -i0.2 -fq $FR2 | awk '{print $7}' | tr '/' ' ' | awk '{print $2}'` 

if [ `echo $NY2_FR2_L | wc -c` == 1 ] 
then 
  NY2_FR2_L=-1;
fi

echo $NY2_FR2_L | awk '{ if ( $1 == -1 ) { print "NY-FR2 PING ERROR"} else if( $1 > ( '$AVG_NY2_FR2' * '$HIGH_PERC_ALERT' ) ) { print "Higher NY FR2 Latency : " $1 " Base Average : " '$AVG_NY2_FR2'} else if( $1 < ( '$AVG_NY2_FR2' * '$LOW_PERC_ALERT' ) ) {print "Lower NY FR2 Latency : " $1 " Base Average : " '$AVG_NY2_FR2'} }' >> $MAIL_FILE

#### TOR11
NY2_TOR_L=`ping -c$PACKETS -i0.2 -fq $TOR | awk '{print $7}' | tr '/' ' ' | awk '{print $2}'`

if [ `echo $NY2_TOR_L | wc -c` == 1 ] 
then 
  NY2_TOR_L=-1;
fi

echo $NY2_TOR_L | awk '{ if ( $1 == -1 ) { print "NY-TOR PING ERROR"} else if( $1 > ( '$AVG_NY2_TOR' * '$HIGH_PERC_ALERT' ) ) { print "Higher NY TOR Latency : " $1 " Base Average : " '$AVG_NY2_TOR'} else if( $1 < ( '$AVG_NY2_TOR' * '$LOW_PERC_ALERT' ) ) {print "Lower NY TOR Latency : " $1 " Base Average : " '$AVG_NY2_TOR'} }' >> $MAIL_FILE

#### BRZ11
NY2_BRZ_L=`ping -c$PACKETS -i0.2 -fq $BRZ | awk '{print $7}' | tr '/' ' ' | awk '{print $2}'` 

if [ `echo $NY2_BRZ_L | wc -c` == 1 ] 
then 
  NY2_BRZ_L=-1;
fi

echo $NY2_BRZ_L | awk '{ if ( $1 == -1 ) { print "NY-BRZ PING ERROR"} else if( ( $1 - '$AVG_NY2_BRZ' ) * 1000 >= '$BRZ_USEC_THRESHOLD' ) { print "Higher NY BRZ Latency : " $1 " Base Average : " '$AVG_NY2_BRZ'} else if( $1 < ( '$AVG_NY2_BRZ' * '$LOW_PERC_ALERT' ) ) {print "Lower NY BRZ Latency : " $1 " Base Average : " '$AVG_NY2_BRZ'} }' >> $MAIL_FILE

LINE_COUNT=`wc -l $MAIL_FILE | awk '{print $1}'`

if [ $(($LINE_COUNT)) -gt 0 ]
then
/bin/mail -s "LatencyAlert" -r "latencyinfo@ny16" "nseall@tworoads.co.in" < $MAIL_FILE
fi

echo `date +"%s"` $NY2_CHI_L $NY2_FR2_L $NY2_TOR_L $NY2_BRZ_L >> $LATENCY_FILE

rm -rf $MAIL_FILE
