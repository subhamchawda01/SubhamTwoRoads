#!/bin/bash

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

while :
do

ps_file="/tmp/mem_usage_ps"
ps -eo pmem,pcpu,rss,vsize,args | sort -k 1 -r |  head -n 6 | grep -v "^%MEM" > $ps_file
top_mem=`head -n1 $ps_file  | awk '{if ($1 > 70 ) print "1" ; else print "0" ; }' `
if [ $top_mem -gt "0" ]; then
SUBJECT="Memory usage on server $(hostname)";
EMAIL="nseall@tworoads.co.in";
/bin/mail -s "$SUBJECT" "$EMAIL" < $ps_file
fi

sleep 30

done

