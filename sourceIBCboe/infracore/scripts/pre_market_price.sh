#!/bin/bash

date_=$1
echo "DATE: $date_"
YYYY=${date_:0:4}
MM=${date_:4:2}
DD=${date_:6:2}
echo "date: $YYYY/$MM/$DD"

list=$(while read -r line; do echo $line | awk '{print substr($1,5)}'; done < /home/dvctrader/ATHENA/pos_exec_file_temp)
if [ ! -e "/spare/local/security${date_}.gz" ] ; then
	echo "security file not present for $date date in /spare/local" 
	if [ ! -e "/spare/local/files/NSEFTPFiles/$YYYY/$MM/$DD/security" ]; then
          echo "security file not present for $date date in /spare/local/files/NSEFTPFiles/"
	  less /home/dvctrader/ATHENA/pos_exec_file_temp | awk '{print $1,0,0}' > dummy2
	  echo "security file not present for $date date" | mail -s "Pre-market Price Alert" -r "${HOSTNAME}-${USER}<hardik.dhakate@tworoads-trading.co.in>" hardik.dhakate@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in
	  exit;
	else
	  echo "extracting info from /spare/local/files/NSEFTPFiles/"
	  for l in $list; do zgrep "|$l|EQ" /spare/local/files/NSEFTPFiles/${YYYY}/${MM}/${DD}/security; done > dummy
          less dummy | awk -F "[|-]" '{print "NSE_"$2,$7, $8}' > dummy2
	fi
else
	echo "extracting info from /spare/local/"
	for l in $list; do zgrep "|$l|EQ" /spare/local/security${date_}.gz; done > dummy
	less dummy | awk -F "[|-]" '{print "NSE_"$2,$7, $8}' > dummy2

fi

join -j 1 -o 1.1,1.2,2.2 /home/dvctrader/ATHENA/pos_exec_file_temp dummy2 > dummy
mv dummy /home/dvctrader/ATHENA/pos_exec_file_temp
rm dummy2
