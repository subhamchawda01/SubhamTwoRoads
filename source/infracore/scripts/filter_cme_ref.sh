#!/bin/bash

#trim the cme ref files
today=`date '+%Y%m%d'`


export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

#read shortcodes list and get its symbol 
for a in `cat /home/dvcinfra/infracore_install/files/CME/cme-shortcodes.txt `
do /home/dvcinfra/infracore_install/bin/get_exchange_symbol $a $today 2>/dev/null; echo "" 
done | sed '/^[ \t]*$/d' > /tmp/regex_for_cme_ref

#filter ref file
grep -w -f /tmp/regex_for_cme_ref /spare/local/files/CME/cme-ref.txt > /spare/local/files/CME/cme-ref.txt_min

#filter mcast file
awk '{print "^"$3}' /spare/local/files/CME/cme-ref.txt_min | sort | uniq  > /tmp/regex_for_cme_ref
grep -w -f /tmp/regex_for_cme_ref /spare/local/files/CME/cme-mcast.txt > /spare/local/files/CME/cme-mcast.txt_min
rm /tmp/regex_for_cme_ref

mv /spare/local/files/CME/cme-ref.txt /spare/local/files/CME/cme-ref.txt_longer
mv /spare/local/files/CME/cme-mcast.txt_min /spare/local/files/CME/cme-ref.txt 

mv /spare/local/files/CME/cme-mcast.txt /spare/local/files/CME/cme-mcast.txt_longer
mv /spare/local/files/CME/cme-mcast.txt_min /spare/local/files/CME/cme-mcast.txt
