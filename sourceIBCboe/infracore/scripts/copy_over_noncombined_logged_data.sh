#!/bin/bash

USAGE1="$0 ALL_FILES_LOCATION GENERIC_DATA_LOCATION DEST_LOCATION DATE"
EXAMP1="$0 /spare/local/MDSlogs/NonCombined/LIFFE /spare/local/MDSlogs/GENERIC /spare/local/MDSlogs/LIFFE TODAY" 

if [ $# -ne 4 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

ALL_FILES_LOC=$1 ; shift ; 
GENERIC_DATA_LOC=$1 ; shift ;
DEST_DATA_LOC=$1 ; shift ; 
YYYYMMDD=$1 ; shift ;

#Get exchange from path
EXCHANGE=`basename $ALL_FILES_LOC`
TEMP_ALL_FILES_LIST=/tmp/all_mds_files.lst."$EXCHANGE" ;
TEMP_GENERIC_FILES_LIST=/tmp/all_generic_data_files.lst."$EXCHANGE" ;
TEMP_DIFF_FILE=/tmp/temp_diff.tmp."$EXCHANGE" ;


if [ "$YYYYMMDD" == "TODAY" ] 
then

   YYYYMMDD=$(date "+%Y%m%d") ;

fi 

ls $ALL_FILES_LOC/*$YYYYMMDD* | awk -F"/" '{print $NF}' | sort | uniq > $TEMP_ALL_FILES_LIST ; 
ls $GENERIC_DATA_LOC/*$YYYYMMDD* | awk -F"/" '{print $NF}' | sort | uniq > $TEMP_GENERIC_FILES_LIST ; 

#order of args is important
awk 'NR==FNR{a[$0]=1;next}!a[$0]' $TEMP_GENERIC_FILES_LIST $TEMP_ALL_FILES_LIST > $TEMP_DIFF_FILE ; 

cd $ALL_FILES_LOC ; 

cp `cat $TEMP_DIFF_FILE | xargs` $DEST_DATA_LOC ; 
rm -rf $TEMP_ALL_FILES_LIST $TEMP_GENERIC_FILES_LIST $TEMP_DIFF_FILE ; 
