#! /bin/bash

CHECK_FOR_DISCREPANCY=`cat /home/dvcinfra/interaday_recon/result | grep -v INTERAPOLATION | grep "DISCREPENCY\|SYMBOL_NOT_FOUND" | grep -v grep`

if [ ! -n "$CHECK_FOR_DISCREPANCY" ]
then
	echo -e $CHECK_FOR_DISCREPANCY | mail -s "Discrepency in ose interaday pnl" "nseall@tworoads.co.in"
fi

CHECK_FOR_DISCREPANCY=`cat /home/dvcinfra/interaday_recon/result | grep VALUES | grep -v grep`

if [ -n "$CHECK_FOR_DISCREPANCY" ]
then
    echo -e " " | mail -s "Not getting data for ose interaday pnl" "nseall@tworoads.co.in"
fi
