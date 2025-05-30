#!/bin/bash
ACCUMULATED_TAGS_DIR="/spare/local/pnl_delta/tags/"
DEST_USER_SERVER="dvcinfra@10.23.74.51"
DEST_DIR="/spare/local/logs/pnl_data/hft/tag_pnl/saci_maps_hist/"
LAST_TAG_FILE="";
time_to_switch_=221000; #TODO
YYYYMMDD=$(date "+%Y%m%d");
LAST_TAG_FILE=$ACCUMULATED_TAGS_DIR`hostname`_qid_saci_tag.$YYYYMMDD

mkdir -p $ACCUMULATED_TAGS_DIR

TRADELOGS_DIR="/spare/local/logs/tradelogs/"; #Path to base directory

while [ true ]
do
    YYYYMMDD=$(date "+%Y%m%d");
    time_now_=$(date "+%H%M%S");
    if [ "$time_now_" -gt "$time_to_switch_" ] ; then
        YYYYMMDD=$(date --date='tomorrow' "+%Y%m%d"); 
        LAST_TAG_FILE=$ACCUMULATED_TAGS_DIR`hostname`_qid_saci_tag.$YYYYMMDD
    fi
    echo $LAST_TAG_FILE;
    cat $TRADELOGS_DIR"qid_saci_tag."*"."$YYYYMMDD > $LAST_TAG_FILE;
    rsync -avz $LAST_TAG_FILE $DEST_USER_SERVER:$DEST_DIR
    sleep 600
done

