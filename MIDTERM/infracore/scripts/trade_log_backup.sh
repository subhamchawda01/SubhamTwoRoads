#!/bin/bash
USAGE="$0    EXCHANGE    PROFILE   YYYYMMDD";

if [ $# -ne 3 ] ;
then
    echo $USAGE
    exit;
fi

USER="dvcinfra"

EXCHANGE=$1;
PROFILE=$2;
YYYYMMDD=$3;

if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
fi

if [ $USER != "dvcinfra" ]
then
    echo "Script must be run as dvcinfra";
    exit 0;
fi

DEST_SERVER="10.23.74.41"
NEWDIR=${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2} 

TRADELOG_DIR="/spare/local/ORSlogs/$EXCHANGE/$PROFILE";

####check if exchange and profile names are valid#####
if [ -d $TRADELOG_DIR ] ; then
	cd $TRADELOG_DIR;
else
	echo "$TRADELOG_DIR does not exist. Recheck inputs. Exiting. ";
	exit;
fi

FILES_TO_BACKUP=(`ls | grep $YYYYMMDD`)
if [ ! -z $FILES_TO_BACKUP ] ; then
	for file in "${FILES_TO_BACKUP[@]}"
	do
	    ###do not gzip trades file, pnls and other assume trades.YYYYMMDD as filename###
	    if [ "$file" == "trades.$YYYYMMDD" ]; then
	        GZIPPED_FILE=$file
	        DESTDIR="/apps/logs/ORSTrades/$EXCHANGE/$PROFILE/$NEWDIR";   	
	    elif [ "$file" == "position.$YYYYMMDD" ]; then
	    	GZIPPED_FILE=$file
	    	DESTDIR="/apps/logs/ORSLogs/$EXCHANGE/$PROFILE/$NEWDIR";
	    else
	      	EXTENSION=`echo "$file" | awk -F"." '{print $NF}'`
	    	if [ "$EXTENSION" != "gz" ]; then
	    		gzip -f $file	#zip all files
	    		GZIPPED_FILE=$file".gz";
	    	else
	    		GZIPPED_FILE=$file
	    	fi
	    	DESTDIR="/apps/logs/ORSLogs/$EXCHANGE/$PROFILE/$NEWDIR";
	    fi
	    
	    ###backup this zipped file on NAS2 as well###
	    ssh $DEST_SERVER -l $USER "mkdir -p $DESTDIR"
    	rsync -avz --quiet $GZIPPED_FILE $USER@$DEST_SERVER:$DESTDIR/
    	ssh $USER@$DEST_SERVER 'bash -s' < ~/LiveExec/scripts/s3_hs1_file_upload.sh $DESTDIR/$GZIPPED_FILE
	done
else
	echo "No logs to backup. Exiting.";
fi
