#!/bin/bash

DIR_LOG="/spare/local/files/tmp"

if [ ! -d "$DIR_LOG" ]; then
 mkdir -p $DIR_LOG
fi
if [ ! -f "$DIR_LOG/alert" ]; then
 touch $DIR_LOG/alert
fi
if [ ! -f "$DIR_LOG/proc_status" ]; then
 touch $DIR_LOG/proc_status
fi


export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

mv $DIR_LOG/alert $DIR_LOG/alert.bak
mv $DIR_LOG/proc_status $DIR_LOG/proc_status.bak

$HOME/LiveExec/scripts/check_proc_status.pl > $DIR_LOG/proc_status

grep "^ALERT" $DIR_LOG/proc_status > $DIR_LOG/alert

diff -u $DIR_LOG/alert $DIR_LOG/alert.bak | grep "^+ALERT" | sed 's/-\?[0-9][0-9]:[0-9][0-9].*_ALERT/_ALERT/' | sed 's/^+ALERT/RECOVERY:/' | sed 's/_ALERT_MSG_\(.*\)_RECO_MSG_\(.*\)/::: \2/' > $DIR_LOG/alert_mail > $DIR_LOG/recovery_mail

diff -u $DIR_LOG/alert $DIR_LOG/alert.bak | grep "^-ALERT" | sed 's/-\?[0-9][0-9]:[0-9][0-9].*_ALERT/_ALERT/' | sed 's/^-ALERT/ALERT:/' |sed 's/_ALERT_MSG_\(.*\)_RECO_MSG_\(.*\)/::: \1/' > $DIR_LOG/alert_mail > $DIR_LOG/alert_mail

   
IS_ALERT_NONEMPTY=$(cat /spare/local/files/tmp/alert | wc -c);
# There is an alert, send an email
if [ $IS_ALERT_NONEMPTY != 0 ]; then
DATE=$(date);
SUBJECT="ALERT : Processes on Trading servers $DATE";
EMAIL="nseall@tworoads.co.in";
/bin/mail -s "$SUBJECT" "$EMAIL" < /spare/local/files/tmp/alert;
fi

IS_RECOVERY_NONEMPTY=$(cat /spare/local/files/tmp/recovery_mail | wc -c);
# There is an alert, send an email
if [ $IS_RECOVERY_NONEMPTY != 0 ]; then
DATE=$(date);
SUBJECT="RECOVERY : Processes on Trading servers $DATE";
EMAIL="nseall@tworoads.co.in";
/bin/mail -s "$SUBJECT" "$EMAIL" < /spare/local/files/tmp/recovery_mail;
fi

cat  /spare/local/files/tmp/alert_mail /spare/local/files/tmp/recovery_mail | sed 's/.*::://; ' | while read a ; do sh ~/LiveExec/scripts/sendAlert.sh $a ; sleep 5  ; done 
