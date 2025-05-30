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

DEST_SERVER="10.23.74.41"; # NAS
DEST_USER="dvcinfra";

if [ $YYYYMMDD = "TODAY" ] ;                                                                                                                                                                                                                then                                                                                                                                                                                                                                        
    YYYYMMDD=$(date "+%Y%m%d");                                                                                                                                                                                                             
fi 

if [ $USER != "dvctrader" ]
then
    echo "Script must be run as dvcinfra";
    exit 0;
fi

HOSTNAME=`hostname` ;

TRADELOGDIR=/spare/local/logs/tradelogs;

TRADES_FILES=$TRADELOGDIR"/trades."$YYYYMMDD".*";
DEST_TMP="/apps" #base folder in DEST_SERVER where files get uploaded
DEST_BASETMP="/NAS1" #base folder in s3 where files get uploaded

DEST_TRD_LOC="logs/QueryTrades/"${YYYYMMDD:0:4}"/"${YYYYMMDD:4:2}"/"${YYYYMMDD:6:2};
DEST_LOG_LOC="logs/QueryLogs/"${YYYYMMDD:0:4}"/"${YYYYMMDD:4:2}"/"${YYYYMMDD:6:2};
ssh $DEST_SERVER -l $DEST_USER 'mkdir -m 777 -p '$DEST_TMP/$DEST_LOG_LOC;

#######
LOGFILES=$TRADELOGDIR"/log."$YYYYMMDD".*";
for name in $LOGFILES ;
do
    if [ -e $name ] ; then
	PID=`ls $name | awk -F '.' '{print $NF}'`;
	if [ $PID == "gz" ]; then
		`gunzip $name`;
#Get the new name of file, after extraction
		new_name=${name%.*};
		name=$new_name;
		PID=`ls $name | awk -F '.' '{print $NF}'`;
	fi

	for STRATFILEPATH in `grep -B 3 STRATEGYLINE $name | grep -v "ClientLoggingSegmentInitializer" | sort | uniq | grep -v "\--" | grep -v STRATEGYLINE `;
	do
	    if [ -e $STRATFILEPATH ] ; then	
		for MODELFILEPATH in `awk '{print $4}' $STRATFILEPATH`;
		do
		    if [ -e $MODELFILEPATH ] ; then
			rsync $MODELFILEPATH $DEST_USER@$DEST_SERVER:$DEST_TMP/$DEST_LOG_LOC/;
			#uploads file to S3 and EC2 at given path
			ssh $DEST_USER@$DEST_SERVER 'bash -s' < /home/pengine/prod/live_scripts/s3_hs1_file_upload.sh $DEST_BASETMP/$DEST_LOG_LOC/`basename $MODELFILEPATH`
		    fi
		done
	
		for PARAMFILEPATH in `awk '{print $5}' $STRATFILEPATH`;
		do
		    if [ -e $PARAMFILEPATH ] ; then
			rsync $PARAMFILEPATH $DEST_USER@$DEST_SERVER:$DEST_TMP/$DEST_LOG_LOC/;
			ssh $DEST_USER@$DEST_SERVER 'bash -s' < /home/pengine/prod/live_scripts/s3_hs1_file_upload.sh $DEST_BASETMP/$DEST_LOG_LOC/`basename $PARAMFILEPATH`
		    fi
		    for INDIVIDUALPARAMPATH in `grep PARAMFILELIST $PARAMFILEPATH | awk '{print $2}'`;
		    do
				if [ -e $INDIVIDUALPARAMPATH ]; then 
				rsync $INDIVIDUALPARAMPATH $DEST_USER@$DEST_SERVER:$DEST_TMP/$DEST_LOG_LOC/;
				ssh $DEST_USER@$DEST_SERVER 'bash -s ' < /home/pengine/prod/live_scripts/s3_hs1_file_upload.sh $DEST_BASETMP/$DEST_LOG_LOC/`basename $INDIVIDUALPARAMPATH`
				fi
			done
		done
		STRATFILEBASE=`basename $STRATFILEPATH`;
		TEMPSTRATFILEPATH=$TRADELOGDIR/$STRATFILEBASE;
	
		cp $STRATFILEPATH $TEMPSTRATFILEPATH; # in case we don't find any logfiles to search starttrading in 
	
		LOGFILEPATH=$name
		if [ -e $LOGFILEPATH ] ; then
		    UNIXSTARTTIME=`grep StartTrading $LOGFILEPATH | head -n1 | awk '{print $1}'`;
		    ESTSTARTTIME=`~/basetrade/scripts/unixtime2nytime.sh $UNIXSTARTTIME | awk '{print $4}' | awk -F: '{print "EST_"$1""$2}'`;
		    awk '{ $6="'$ESTSTARTTIME'"; print }' $STRATFILEPATH > $TEMPSTRATFILEPATH;
		fi
		LOGFILEGZPATH=$TRADELOGDIR"/log."$YYYYMMDD"."$PID".gz";
		if [ -e $LOGFILEGZPATH ] ; then
		    UNIXSTARTTIME=`zgrep StartTrading $LOGFILEGZPATH | head -n1 | awk '{print $1}'`;
		    ESTSTARTTIME=`~/basetrade/scripts/unixtime2nytime.sh $UNIXSTARTTIME | awk '{print $4}' | awk -F: '{print "EST_"$1""$2}'`;
		    awk '{ $6="'$ESTSTARTTIME'"; print }' $STRATFILEPATH > $TEMPSTRATFILEPATH;
		fi
		
		if [ -e $TEMPSTRATFILEPATH ] ; then
		    rsync $TEMPSTRATFILEPATH $DEST_USER@$DEST_SERVER:$DEST_TMP/$DEST_LOG_LOC/;
		    ssh $DEST_USER@$DEST_SERVER 'bash -s' < /home/pengine/prod/live_scripts/s3_hs1_file_upload.sh $DEST_BASETMP/$DEST_LOG_LOC/`basename $TEMPSTRATFILEPATH`
		fi
	    fi
	done 
        for STRATFILEPATH in `zgrep -B 3 STRATEGYLINE $name | grep -v "ClientLoggingSegmentInitializer" | sort | uniq | grep -v "\--" | grep -v STRATEGYLINE | grep -v PNLSPLIT  | grep -v STRUCTURED_TRADING `;
        do 
          if [ -e $STRATFILEPATH ] ; then 
            for IM_STRATFILEPATH in `awk '{print $2}' $STRATFILEPATH`; 
            do
              if [ -e $IM_STRATFILEPATH ]; then 
                for MODELFILEPATH in `cat $IM_STRATFILEPATH | grep -v STRUCTURED_TRADING | awk '{print $3}'`
                do
                  if [ -e $MODELFILEPATH ] ; then
                    rsync $MODELFILEPATH $DEST_USER@$DEST_SERVER:$DEST_TMP/$DEST_LOG_LOC/;
		    ssh $DEST_USER@$DEST_SERVER 'bash -s' < /home/pengine/prod/live_scripts/s3_hs1_file_upload.sh $DEST_BASETMP/$DEST_LOG_LOC/`basename $MODELFILEPATH`
                  fi
                done

                for PARAMFILEPATH in `cat $IM_STRATFILEPATH | grep -v STRUCTURED_TRADING | awk '{print $4}'`
                do
                  if [ -e $PARAMFILEPATH ] ; then 
                    rsync $PARAMFILEPATH $DEST_USER@$DEST_SERVER:$DEST_TMP/$DEST_LOG_LOC/
		    ssh $DEST_USER@$DEST_SERVER 'bash -s' < /home/pengine/prod/live_scripts/s3_hs1_file_upload.sh $DEST_BASETMP/$DEST_LOG_LOC/`basename $PARAMFILEPATH`
                  fi
                done
                rsync $IM_STRATFILEPATH $DEST_USER@$DEST_SERVER:$DEST_TMP/$DEST_LOG_LOC/;
		ssh $DEST_USER@$DEST_SERVER 'bash -s' < /home/pengine/prod/live_scripts/s3_hs1_file_upload.sh $DEST_BASETMP/$DEST_LOG_LOC/`basename $IM_STRATFILEPATH`
              fi
             done
             rsync $STRATFILEPATH $DEST_USER@$DEST_SERVER:$DEST_TMP/$DEST_LOG_LOC/;
	     ssh $DEST_USER@$DEST_SERVER 'bash -s' < /home/pengine/prod/live_scripts/s3_hs1_file_upload.sh $DEST_BASETMP/$DEST_LOG_LOC/`basename $STRATFILEPATH`
            fi 
        done 
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


# Create the directory if needed.
ssh $DEST_SERVER -l $DEST_USER 'mkdir -p '$DEST_TMP/$DEST_TRD_LOC;
scp -q $TRADES_FILES $DEST_USER@$DEST_SERVER:$DEST_TMP/$DEST_TRD_LOC/;

for TRADE_FILE in $TRADES_FILES ;
do
	ssh $DEST_USER@$DEST_SERVER 'bash -s' < /home/pengine/prod/live_scripts/s3_hs1_file_upload.sh $DEST_BASETMP/$DEST_TRD_LOC/`basename $TRADE_FILE`
done

scp -q $GZIPPED_LOGFILES $DEST_USER@$DEST_SERVER:$DEST_TMP/$DEST_LOG_LOC/;

for GZIPPED_LOGFILE in $GZIPPED_LOGFILES ;
do
	ssh $DEST_USER@$DEST_SERVER 'bash -s' < /home/pengine/prod/live_scripts/s3_hs1_file_upload.sh $DEST_BASETMP/$DEST_LOG_LOC/`basename $GZIPPED_LOGFILE`
done

#if [ $CLEANUP = "Y" ] ;
#then
#find $TRADELOGDIR -name log.\* -type f -mtime +15 -exec rm '{}' \; 
#fi
