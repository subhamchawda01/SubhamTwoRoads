#!/bin/bash

#YYYYMMDD=`date +%Y%m%d`;

USAGE="$0 START_DATE END_DATE";
if [ $# -lt 2 ] ; 
then 
    echo $USAGE;
    exit;
fi

STARTDATE=$1; shift; 
ENDDATE=$1; shift;

QUERYTRADESDIR=/NAS1/logs/QueryTrades
QUERYLOGDIR=/NAS1/logs/QueryLogs

YYYYMMDD=$STARTDATE;
NUMDAYS=0;

while [ $NUMDAYS -lt 620 ] 
do
    NEWDIR=${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2};

    FULL_QUERY_TRADES_DIR=$QUERYTRADESDIR/$NEWDIR;
    FULL_QUERY_LOG_DIR=$QUERYLOGDIR/$NEWDIR;
    
    for TRADESFILENAME in $FULL_QUERY_TRADES_DIR/trades.$YYYYMMDD.* ; 
    do
	if [ -e $TRADESFILENAME ] ; 
	then 
	    t_pnl_stats=`~/basetrade/ModelScripts/get_pnl_stats_2.pl $TRADESFILENAME H | cut -d' ' -f1,2,3 | sed 's/ /:/g'`;
	    for t_pstats in $t_pnl_stats ; do
		REALID=${t_pstats%%:*}; prest=${t_pstats#*:}; prest=${prest//:/ };
		
		POSSIBLEFILENAMES=`ls -altr $FULL_QUERY_LOG_DIR/*_$REALID 2>/dev/null`;
		if [ $? -gt 0 ] ; then
		    POSSIBLEFILENAMES=`ls -altr $FULL_QUERY_LOG_DIR/*$REALID* 2>/dev/null`;
		    if [ $? -lt 1 ] ; then
			for posfilename in $FULL_QUERY_LOG_DIR/*$REALID* ; do
			    POSSIBLEFILENAMES=`cat $posfilename | awk -vposfilename=$posfilename '{ if ( $8 == '$REALID' ) { print $posfilename; } }'`;
			done
		    fi
		fi
		STRATFILE=`echo $POSSIBLEFILENAMES | awk '{print $9}' | head -n1`;
		echo "$REALID $STRATFILE $prest";
	    done
	fi;
    done

    NUMDAYS=$(( $NUMDAYS + 1 ));
    
    MMDD=${YYYYMMDD:4:4};
    if [ $MMDD -eq "1231" ] ; then
	YYYYMMDD=$(( $YYYYMMDD + 8870 )); # 20120101 - 20111231
    else
	DD=${YYYYMMDD:6:2};
	if [ $DD -eq "31" ]; then
	    YYYYMMDD=$(( $YYYYMMDD + 70 )); # 20120201 - 20120131
	else
	    YYYYMMDD=$(( $YYYYMMDD + 1 ));
	fi
    fi
    
    if [ $YYYYMMDD -gt $ENDDATE ] ; then
	NUMDAYS=1000;
    fi
done
