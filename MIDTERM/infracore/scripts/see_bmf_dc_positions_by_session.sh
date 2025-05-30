#!/bin/bash

USAGE1="$0 DC_SESSION DATE"
EXAMPLE1="$0 DALP0012 20130525"

if [ $# -lt 2 ] ;
then
    echo "USAGE : ";
    echo $USAGE1;
    echo $EXAMPLE1;
    exit;
fi


export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

DCS=$1 ; shift ;
YYYYMMDD=$1 ; shift ;

ORSDIR=/spare/local/ORSlogs/BMFEP/ ;
FIXLOG="OPTIMUMFIX.4.4.messages.log" ;

TEMP_PROD_FILE=/tmp/products_traded.tmp 
TEMP_ACCOUNTS_FILE=/tmp/accounts_used.tmp 
TEMP_SESSIONS_FILE=/tmp/sessions_used.tmp 
TEMP_STATS_FILE=/tmp/dc_stats.tmp 
CALC=$HOME/infracore/scripts/sumcalc.pl 

>$TEMP_STATS_FILE ;

grep "$YYYYMMDD" $ORSDIR/$DCS/$FIXLOG | tr '\001' ' ' | grep " 55=" |  awk -F"55=" '{print $2}' | awk '{print $1}' | sort | uniq > $TEMP_PROD_FILE ;  
grep "$YYYYMMDD" $ORSDIR/$DCS/$FIXLOG | tr '\001' ' ' | grep " 1=" |  awk -F"1=" '{print $2}' | awk '{print $1}' | sort | uniq > $TEMP_ACCOUNTS_FILE ;  
grep "$YYYYMMDD" $ORSDIR/$DCS/$FIXLOG | tr '\001' ' ' | grep " 1180=" |  awk -F"1180=" '{print $2}' | awk '{print $1}' | sort | uniq > $TEMP_SESSIONS_FILE ;  

echo "======================================= PER SESSION SUMMARY =======================================" >> $TEMP_STATS_FILE  
echo >> $TEMP_STATS_FILE; echo >> $TEMP_STATS_FILE ; echo >> $TEMP_STATS_FILE ; 

for session in `cat $TEMP_SESSIONS_FILE`
do

    for product in `cat $TEMP_PROD_FILE` 
    do 

	NETBUY=`grep "$YYYYMMDD" $ORSDIR/$DCS/$FIXLOG | grep "$session" | grep "$product" | tr '\001' ' ' | grep "35=8" | egrep "39=1|39=2" | grep "54=1" | awk '{print $15}' | awk -F"=" '{print $2}'  | $CALC` ;
	NETSELL=`grep "$YYYYMMDD" $ORSDIR/$DCS/$FIXLOG | grep "$session" | grep "$product" | tr '\001' ' ' | grep "35=8" | egrep "39=1|39=2" | grep "54=2" | awk '{print $15}' | awk -F"=" '{print $2}'  | $CALC` ;
	NET=$((NETBUY-NETSELL)) ;

	echo "SESSION -> " $session " PRODUCT : " $product " BUYS -> " $NETBUY " SELL -> " $NETSELL " NET -> " $NET >> $TEMP_STATS_FILE ;

    done 

done 

echo >> $TEMP_STATS_FILE; echo >> $TEMP_STATS_FILE ; 
echo "==================================================================================================" >> $TEMP_STATS_FILE
echo >> $TEMP_STATS_FILE; echo >> $TEMP_STATS_FILE ; echo >> $TEMP_STATS_FILE ; 

echo "======================================= PER ACCOUNT SUMMARY =======================================" >> $TEMP_STATS_FILE  
echo >> $TEMP_STATS_FILE; echo >> $TEMP_STATS_FILE ; echo >> $TEMP_STATS_FILE ; 

for account in `cat $TEMP_ACCOUNTS_FILE`
do

    for product in `cat $TEMP_PROD_FILE` 
    do 

	NETBUY=`grep "$YYYYMMDD" $ORSDIR/$DCS/$FIXLOG | grep "$account" | grep "$product" | tr '\001' ' ' | grep "35=8" | egrep "39=1|39=2" | grep "54=1" | awk '{print $15}' | awk -F"=" '{print $2}'  | $CALC` ;
	NETSELL=`grep "$YYYYMMDD" $ORSDIR/$DCS/$FIXLOG | grep "$account" | grep "$product" | tr '\001' ' ' | grep "35=8" | egrep "39=1|39=2" | grep "54=2" | awk '{print $15}' | awk -F"=" '{print $2}'  | $CALC` ;
	NET=$(($NETBUY-$NETSELL)) ;

	echo "ACCOUNT -> " $account " PRODUCT : " $product " BUYS -> " $NETBUY " SELL -> " $NETSELL " NET -> " $NET >> $TEMP_STATS_FILE ;

    done 

done 

echo >> $TEMP_STATS_FILE; echo >> $TEMP_STATS_FILE ; 
echo "==================================================================================================" >> $TEMP_STATS_FILE 
echo >> $TEMP_STATS_FILE; echo >> $TEMP_STATS_FILE ; echo >> $TEMP_STATS_FILE ; 

echo "======================================= PER PRODUCT SUMMARY =======================================" >> $TEMP_STATS_FILE  
echo >> $TEMP_STATS_FILE; echo >> $TEMP_STATS_FILE ; echo >> $TEMP_STATS_FILE ; 

for product in `cat $TEMP_PROD_FILE`
do

    NETBUY=`grep "$YYYYMMDD" $ORSDIR/$DCS/$FIXLOG | grep "$product" | tr '\001' ' ' | grep "35=8" | egrep "39=1|39=2" | grep "54=1" | awk '{print $15}' | awk -F"=" '{print $2}'  | $CALC` ;
    NETSELL=`grep "$YYYYMMDD" $ORSDIR/$DCS/$FIXLOG | grep "$product" | tr '\001' ' ' | grep "35=8" | egrep "39=1|39=2" | grep "54=2" | awk '{print $15}' | awk -F"=" '{print $2}'  | $CALC` ;
    NET=$(($NETBUY-$NETSELL)) ;

    echo "PRODUCT -> " $product " BUYS -> " $NETBUY " SELL -> " $NETSELL " NET -> " $NET >> $TEMP_STATS_FILE ; 

done 

echo >> $TEMP_STATS_FILE; echo >> $TEMP_STATS_FILE ; 
echo "==================================================================================================" >> $TEMP_STATS_FILE ;

cat $TEMP_STATS_FILE ; 

rm -rf $TEMP_PROD_FILE ;
rm -rf $TEMP_ACCOUNTS_FILE ; 
rm -rf $TEMP_SESSIONS_FILE ; 
rm -rf $TEMP_STATS_FILE ;
