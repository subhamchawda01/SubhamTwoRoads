#!/bin/bash

YYYYMMDD=`date +%Y%m%d`;

USAGE="$0 [YYYYMMDD=TODAY] [TRADEDATE=$YYYYMMDD] [PRODUCT_NAME]";
echo "No args: $#";
if [ $# -ge 1 ] ; 
then 
    YYYYMMDD=$1; shift;
    echo "YYYYMMDD = $YYYYMMDD";
fi
TRADEDATE=$YYYYMMDD;
PRODUCT_NAME="";
if [ $# -ge 1 ] ; 
then 
    PRODUCT_NAME=$1; shift;
fi
echo "Product name to process: $PRODUCT_NAME";

NEWDIR=${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}

QUERYTRADEDIR=/NAS1/logs/QueryTrades;

if [ "$PRODUCT_NAME" == "" ] ;
#Process all products
then 
    for name in $QUERYTRADEDIR/$NEWDIR/trades.$YYYYMMDD.* ; 
    do 
	echo $name
	PID=`echo $name | awk -F\. '{print $3}'`; 
	echo $PID ;
	~/basetrade/scripts/run_sim_only_with_pca.pl $TRADEDATE $PID; 
    done
    
else
#Process only FESX or ZF etc
    for name in $QUERYTRADEDIR/$NEWDIR/trades.$YYYYMMDD.* ; 
    do 
	IF_PRODUCT_PRESENT_INTRADE_FILE=`grep  $PRODUCT_NAME $name | wc -l`;
	if [ $IF_PRODUCT_PRESENT_INTRADE_FILE -ge 1 ];
	then 
	    PID=`echo $name | awk -F\. '{print $3}'`; 
	    echo $PID
	    ~/basetrade/scripts/run_sim_only_with_pca.pl $TRADEDATE $PID; 
	fi
	
    done
    
	fi