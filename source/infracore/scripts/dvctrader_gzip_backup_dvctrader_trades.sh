#!/bin/bash

# USAGE:
# 
# gunzips the logs saved to the directories in '/spare/local/logs/tradelogs/'
# Sends them over to dev ny4 in /apps/logs/QueryTrades/YYYY/MM/DD

#USAGE="$0    USER     YYYYMMDD     GZIP(Y/N)    CLEANUP(Y/N)";
USAGE="$0 YYYYMMDD=TODAY";
# USAGE_DESC="Gzips & syncs trading logs dated 'YYYYMMDD' to /apps/logs/QueryTrades/YYYY/MM/DD on USER @ ny4-server. Gzipping is controlled by GZIP (Y = Yes, N = No). CLEANUP (Y = Yes, N = No) will erase all trades files older than 15 days.";

if [ $# -lt 1 ] ;
then
    echo $USAGE;
#    echo $USAGE_DESC;
    exit;
fi

YYYYMMDD=$1;
#USER=$1;
#YYYYMMDD=$2;
#GZIP=$3;
#CLEANUP=$4;

DEST_SERVER="10.23.74.40"; # NAS
DEST_USER="dvcinfra";

if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d");
fi

if [ $USER != "dvctrader" ]
then
    echo "Script must be run as dvctrader";
    exit 0;
fi

TRADELOGDIR=/spare/local/logs/tradelogs;

TRADES_FILES=$TRADELOGDIR"/trades."$YYYYMMDD".*";
DEST_TRD_LOC="/apps/logs/QueryTrades/"${YYYYMMDD:0:4}"/"${YYYYMMDD:4:2}"/"${YYYYMMDD:6:2};
DEST_LOG_LOC="/apps/logs/QueryLogs/"${YYYYMMDD:0:4}"/"${YYYYMMDD:4:2}"/"${YYYYMMDD:6:2};
ssh $DEST_SERVER -l $DEST_USER 'mkdir -p '$DEST_LOG_LOC;

#######

for STRATFILEPATH in `crontab -l | grep start | awk '{print $9}'`; 
do 
    if [ -e $STRATFILEPATH ] ; then
	PID=`echo $STRATFILEPATH | awk -F\_ '{ print $NF }'`;
	LOGFILEPATH=$TRADELOGDIR"/log."$YYYYMMDD"."$PID;
	if [ -e $LOGFILEPATH ] ; then
	    
	    MODELFILEPATH=`awk '{print $4}' $STRATFILEPATH`;
	    if [ -e $MODELFILEPATH ] ; then
		rsync $MODELFILEPATH $DEST_USER@$DEST_SERVER:$DEST_LOG_LOC/;

		PARAMFILEPATH=`awk '{print $5}' $STRATFILEPATH`;
		if [ -e $PARAMFILEPATH ] ; then
		    rsync $PARAMFILEPATH $DEST_USER@$DEST_SERVER:$DEST_LOG_LOC/;
		    
		    STRATFILEBASE=`basename $STRATFILEPATH`;
		    TEMPSTRATFILEPATH=$TRADELOGDIR/$STRATFILEBASE;
		    
		    UNIXSTARTTIME=`grep StartTrading $LOGFILEPATH | head -n1 | awk '{print $1}'`;
		    ESTSTARTTIME=`~/basetrade/scripts/unixtime2nytime.sh $UNIXSTARTTIME | awk '{print $4}' | awk -F: '{print "EST_"$1""$2}'`;

		    awk '{ $6="'$ESTSTARTTIME'"; print }' $STRATFILEPATH > $TEMPSTRATFILEPATH;
		    if [ -e $TEMPSTRATFILEPATH ] ; then
#			echo $ESTSTARTTIME;
			rsync $TEMPSTRATFILEPATH $DEST_USER@$DEST_SERVER:$DEST_LOG_LOC/;
		    fi
		fi
	    fi
	fi
    fi
done 

#######

LOGFILES=$TRADELOGDIR"/log."$YYYYMMDD".*";
for name in $LOGFILES ;
do
    if [ -f $name ] ; then
	gzip $name ;
    fi
done
GZIPPED_LOGFILES=$TRADELOGDIR"/log."$YYYYMMDD".*.gz";

# echo $DEST_TRD_LOC

# Create the directory if needed.
ssh $DEST_SERVER -l $DEST_USER 'mkdir -p '$DEST_TRD_LOC;
scp -q $TRADES_FILES $DEST_USER@$DEST_SERVER:$DEST_TRD_LOC/;

scp -q $GZIPPED_LOGFILES $DEST_USER@$DEST_SERVER:$DEST_LOG_LOC/;

#if [ $CLEANUP = "Y" ] ;
#then
find $TRADELOGDIR -name log.\* -type f -mtime +15 -exec rm '{}' \; 
#fi
