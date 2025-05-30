#!/bin/bash

USAGE1="$0 Threshold"
EXAMP1="$0 4000"

if [ $# -ne 1 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

THRSHOLD=$1 ; 

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


bsl11fm=`ssh 10.23.52.51 'free -m | grep "buffer" | tail -1' | awk '{print $4}'` ;
bsl12fm=`ssh 10.23.52.52 'free -m | grep "buffer" | tail -1' | awk '{print $4}'` ;
bsl13fm=`ssh 10.23.52.53 'free -m | grep "buffer" | tail -1' | awk '{print $4}'` ;


hk11fm=`ssh 10.152.224.145 'free -m | grep "buffer" | tail -1' | awk '{print $4}'` ;
hk12fm=`ssh 10.152.224.146 'free -m | grep "buffer" | tail -1' | awk '{print $4}'` ;

MAIL_FILE=/tmp/free_apps_memory.txt 
touch $MAIL_FILE
>$MAIL_FILE


if [ $bsl11fm -lt $THRSHOLD ] 
then 

    echo "BSL 11 Free Apps Memory Below Threshold : " $bsl11fm " Threshold Value : " $THRSHOLD >> $MAIL_FILE ;
    
fi 

if [ $bsl12fm -lt $THRSHOLD ] 
then 

    echo "BSL 12 Free Apps Memory Below Threshold : " $bsl12fm " Threshold Value : " $THRSHOLD >> $MAIL_FILE ;
    
fi 

if [ $bsl13fm -lt $THRSHOLD ] 
then 

    echo "BSL 13 Free Apps Memory Below Threshold : " $bsl13fm " Threshold Value : " $THRSHOLD >> $MAIL_FILE ;
    
fi 

if [ $hk11fm -lt $THRSHOLD ] 
then 

    echo "HK 11 Free Apps Memory Below Threshold : " $hk11fm " Threshold Value : " $THRSHOLD >> $MAIL_FILE ;
    
fi 

if [ $hk12fm -lt $THRSHOLD ] 
then 

    echo "HK 12 Free Apps Memory Below Threshold : " $hk12fm " Threshold Value : " $THRSHOLD >> $MAIL_FILE ;
    
fi 


LINE_COUNT=`wc -l $MAIL_FILE | awk '{print $1}'`

if [ $(($LINE_COUNT)) -gt 0 ]
then
    /bin/mail -s "VMA Server Free Apps Memory Alert" -r "usermemorymonitor@ny11" "nseall@tworoads.co.in" < $MAIL_FILE
fi

rm -rf $MAIL_FILE

