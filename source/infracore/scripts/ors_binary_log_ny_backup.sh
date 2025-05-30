#!/bin/bash

USAGE="$0 YYYYMMDD";
USAGE_DESC="Copy ORS bin logs from /spare to /apps";

if [ $# -ne 1 ] ;
then
    echo $USAGE
    echo $USAGE_DESC
    exit;
fi

YYYYMMDD=$1
LOG_LOC=/spare/local/ORSBCAST
COPY_LOC=/NAS1/data/ORSData/NY4

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
fi

for EXCH in CME EUREX TMX BMF
do

  if [ `ls $LOG_LOC/$EXCH | grep $YYYYMMDD | wc -l` -gt 0 ]
  then 

    if [ ! -d $COPY_LOC/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2} ] ;
    then     

      mkdir -p $COPY_LOC/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2} ;    
    
    fi

    gzip $LOG_LOC/$EXCH/*$YYYYMMDD*

    mv $LOG_LOC/$EXCH/*$YYYYMMDD* $COPY_LOC/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}   

  fi
    
done
