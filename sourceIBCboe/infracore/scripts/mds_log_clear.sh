#!/bin/bash

# USAGE:
# 
# Clears the logs for the specified exchange and the specified date.

USAGE="$0    EXCHANGE    YYYYMMDD";
USAGE_DESC="Clears logs for exchange 'EXCHANGE'(ALL) dated 'YYYYMMDD'.";

if [ $# -ne 2 ];
then
    echo $USAGE
    echo $USAGE_DESC
    exit;
fi

EXCHANGE=$1;
YYYYMMDD=$2;

if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
fi

#to clear data for specific exchange
clear_data_for_a_exchange () {
	
	MDSLOG_DIR_=$1;
	shift
	EXCHANGE_=$1;
	
	##Very imp check. If invalid fields, we may end removing everything at some random location(say home)##
	if [[ -z $MDSLOG_DIR_ || -z $EXCHANGE_ ]] ; then
		echo $0" : empty directory paths. Exiting!!"
		exit;
	elif [[ ! -d $MDSLOG_DIR_"/"$EXCHANGE_ ]] ; then
		echo $0" : invalid directory path. Exiting!!"
		exit;		
	fi	
	echo $0" : Moving into " $MDSLOG_DIR_"/"$EXCHANGE_
	
	cd $MDSLOG_DIR_
	cd $EXCHANGE_
	
	echo $0" : erasing all logs for "$EXCHANGE_" for "$YYYYMMDD
	if [ "$EXCHANGE_" == "Raw" ] ;
	then
	    echo "deleting files older than 5hrs"
	    find $MDSLOG_DIR_"/"$EXCHANGE_"/" -type f -mmin +300 -exec rm -f {} \;
	fi
	#rm -f *$YYYYMMDD Not doing this. Worried we'll lose data.
	
	echo $0" : erasing all files for "$EXCHANGE_" older than 2 days"
	find $MDSLOG_DIR_"/"$EXCHANGE_"/" -type f -mtime +2 -exec rm -f {} \;
	
	NonCombined_DIR=$MDSLOG_DIR_"/NonCombined/"$EXCHANGE_;
	if [ -d $NonCombined_DIR ]
	then
	    echo $0" : Moving into " $NonCombined_DIR
	    cd $NonCombined_DIR
	    echo $0" : erasing all files for "$EXCHANGE_" older than 1 days"
	    find $NonCombined_DIR"/" -type f -mtime +1 -exec rm -f {} \;
	fi
	
	echo $0" : CLEARDATA completed for $MDSLOG_DIR_"/"$EXCHANGE_"
}

ERRORFILE=/tmp/$EXCHANGE"_"$YYYYMMDD;

if [ -f $ERRORFILE ] ;
then
    # Existence of this file means that backup failed.
    # DO NOT DELETE THE DATA FILES.
    echo $0" : CLEARDATA FAILED because COPYDATA FAILED. Run COPYDATA again and retry CLEARDATA."
    exit;
fi

MDSLOG_DIR="/spare/local/MDSlogs";

#if cleardata to be done for all exachanges

if [ "$EXCHANGE" == "ALL" ] ; then
	ALL_DIR=(`ls -l "$MDSLOG_DIR" | grep '^d' | awk '{print $NF}'`)
	if [ ! -z $ALL_DIR ] ; then
		for exch_dir in "${ALL_DIR[@]}"
		do
	    	clear_data_for_a_exchange $MDSLOG_DIR $exch_dir
		done
	fi
else
	clear_data_for_a_exchange $MDSLOG_DIR $EXCHANGE
fi
