#!/bin/bash

USAGE="$0 YYYYMMDD";

if [ $# -ne 1 ] ;
then
    echo $USAGE;
    exit;
fi

YYYYMMDD=$1
if [ $YYYYMMDD = "TODAY" ] ;
  then
    YYYYMMDD=$(date "+%Y%m%d")
elif [ $YYYYMMDD = "YESTERDAY" ] ;
  then
    YYYYMMDD=$(cat /tmp/YESTERDAY_DATE)
fi

PNL_RECON_RESULT="$HOME/recon/pnl_recon_result"
TEMP_PNL_RECON_RESULT="$HOME/recon/pnl_recon_result_temp"
PNL_RECON_ERROR="$HOME/recon/pnl_recon_error"
FETCH_SCRIPT=$HOME/infracore/recon/fetch_newedge_files.sh

abn_result="$HOME/recon/abnamro_result"
bcs_result="$HOME/recon/bcs_result"
edf_result="$HOME/recon/edf_result"
newedge_result="$HOME/recon/newedge_result"
plural_result="$HOME/recon/plural_result"


$FETCH_SCRIPT `date "+%Y%m%d"`

flag=0;
> $PNL_RECON_RESULT
> $TEMP_PNL_RECON_RESULT
recon_error_string="Pnl Reconciliation failed for: "
recon_success_sting="Pnl Reconciliation succeeded: "

/home/dvcinfra/infracore/recon/recon_abnamro_trades.sh $YYYYMMDD >> /spare/local/logs/cronlogs/output_logs 2>>/spare/local/logs/cronlogs/error_logs

##check if recon is success or not##
if [ `grep "#FF0000" $abn_result | wc -l` -gt 0 ]; then
	recon_error_string=$recon_error_string" Abnamro  ";
	flag=1;
fi
sleep 10

/home/dvcinfra/infracore/recon/recon_edf_trades.sh $YYYYMMDD >> /spare/local/logs/cronlogs/output_logs 2>>/spare/local/logs/cronlogs/error_logs
##check if recon is success or not##
if [ `grep "#FF0000" $edf_result| wc -l` -gt 0 ]; then
	recon_error_string=$recon_error_string" Edf  ";
	flag=1;
fi
sleep 10

/home/dvcinfra/infracore/recon/recon_newedge_trades.sh $YYYYMMDD >>/spare/local/logs/cronlogs/output_logs 2>>/spare/local/logs/cronlogs/error_logs
##check if recon is success or not##
if [ `grep "#FF0000" $newedge_result | wc -l` -gt 0 ]; then
	recon_error_string=$recon_error_string" Newedge  ";
	flag=1;
fi
sleep 10

/home/dvcinfra/infracore/recon/recon_bcs_trades.sh $YYYYMMDD >>/spare/local/logs/cronlogs/output_logs 2>>/spare/local/logs/cronlogs/error_logs
##check if recon is success or not##
if [ `grep "#FF0000" $bcs_result | wc -l` -gt 0 ]; then
	recon_error_string=$recon_error_string" Bcs  ";
	flag=1;
fi
sleep 10

/home/dvcinfra/infracore/recon/recon_plural_trades.sh $YYYYMMDD >>/spare/local/logs/cronlogs/output_logs 2>>/spare/local/logs/cronlogs/error_logs
##check if recon is success or not##
if [ `grep "#FF0000" $plural_result | wc -l` -gt 0 ]; then
	recon_error_string=$recon_error_string" Plural  ";
	flag=1;
fi


echo "To: abhishek.anand@tworoads.co.in" >> $TEMP_PNL_RECON_RESULT
if [ "$flag" -eq 0 ]; then
	echo "Subject: $recon_success_sting `cat /tmp/YESTERDAY_DATE`" >> $TEMP_PNL_RECON_RESULT
else
	echo "Subject: $recon_error_string `cat /tmp/YESTERDAY_DATE`" >> $TEMP_PNL_RECON_RESULT
fi

echo "Mime-Version: 1.0" >> $TEMP_PNL_RECON_RESULT
echo "Content-Type: text/html" >> $TEMP_PNL_RECON_RESULT
echo "<html><body>" >> $TEMP_PNL_RECON_RESULT

echo "<br> <br> <table border=2>" >> $TEMP_PNL_RECON_RESULT
cat $PNL_RECON_RESULT >> $TEMP_PNL_RECON_RESULT
cat $TEMP_PNL_RECON_RESULT > $PNL_RECON_RESULT
echo "</table>" >> $PNL_RECON_RESULT
echo "</body></html>" >> $PNL_RECON_RESULT

cat $PNL_RECON_RESULT | /usr/sbin/sendmail -t
