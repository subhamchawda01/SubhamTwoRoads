#!/bin/bash

USAGE1="$0 SHORTCODE DATE QUERYID"
EXAMP1="$0 KFFTI_0 20121210 50019"

if [ $# -ne 3 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

SHORTCODE=$1 ; shift ;
YYYYMMDD=$1 ; shift ;
QUERYID=$1 ; shift ;

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


QUERYLOGSDIR=/NAS1/logs/QueryLogs/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2} 

QUERYLOG=log.$YYYYMMDD"."$QUERYID".gz"

if [ ! -f $QUERYLOGSDIR/$QUERYLOG ] 
then

    echo "QUERY LOG" $QUERYLOGSDIR/$QUERYLOG " NOT AVAILABLE FOR EXTRACTING SACI" ;
    exit ;

fi 

FILLRATIO_EXEC=$HOME/infracore_install/bin/get_fillratio_for_shortcode_by_saci 

SACI=`zgrep "SACI" $QUERYLOGSDIR/$QUERYLOG | awk -F"SACI:" '{print $2}' | awk '{print $1}'  | head -1` ;

echo "QUERY : "$QUERYID " DATE : " $YYYYMMDD " FILLRATIO : " `$FILLRATIO_EXEC $SHORTCODE $YYYYMMDD $SACI`

