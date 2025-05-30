#!/bin/bash

YYYYMMDD=`echo $1 | awk -F"_" '{print $2}'` ;

DD=${YYYYMMDD:6:2}
MM=${YYYYMMDD:4:2}
YY=${YYYYMMDD:2:2}
YYYY=${YYYYMMDD:0:4}

FTP_DIR="/spare/local/files/NSEFTPFiles/$YYYY/$MM/$DD" ;
rm -f $FTP_DIR/* ;

/home/dvctrader/LiveExec/scripts/fetch_nse_daily_files.sh $YYYYMMDD >/tmp/nse_ref.err 2>&1 & 
