#!/bin/bash

USAGE1="$0 Threshold"
EXAMP1="$0 0(unsed for now)"

if [ $# -ne 1 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


bsl11sg=`ssh 10.23.52.51 'ipcs -u | grep "pages allocated"' | awk '{print $NF}'` ;
bsl12sg=`ssh 10.23.52.52 'ipcs -u | grep "pages allocated"' | awk '{print $NF}'` ;
bsl13sg=`ssh 10.23.52.53 'ipcs -u | grep "pages allocated"' | awk '{print $NF}'` ;

MAIL_FILE=/tmp/ping_latency_out.txt
touch $MAIL_FILE
>$MAIL_FILE


if [ $bsl11sg -gt 0 ] 
then 

    echo "BSL-11 Shm Pages Allocated At EOD : " $bsl11sg >> $MAIL_FILE ;
    
fi 

if [ $bsl12sg -gt 0 ] 
then 

    echo "BSL-12 Shm Pages Allocated At EOD : " $bsl12sg >> $MAIL_FILE ;
    
fi 

if [ $bsl13sg -gt 0 ] 
then 

    echo "BSL-13 Shm Pages Allocated At EOD : " $bsl13sg >> $MAIL_FILE ;
    
fi 


LINE_COUNT=`wc -l $MAIL_FILE | awk '{print $1}'`

if [ $(($LINE_COUNT)) -gt 0 ]
then
    /bin/mail -s "BSL Server Undetached Shm Segments Alert" -r "shmpagesmonitor@ny11" "nseall@tworoads.co.in" < $MAIL_FILE
fi

rm -rf $MAIL_FILE

