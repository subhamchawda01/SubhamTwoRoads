#!/bin/bash

DIR_LOG="/spare/local/files/tmp"

if [ ! -d "$DIR_LOG" ]; then
 mkdir $DIR_LOG
fi
if [ ! -f "DIR_LOG/alert ]; then
 touch $DIR_LOG/alert
fi
if [ ! -f "DIR_LOG/alert ]; then
 touch $DIR_LOG/proc_status
fi

mv $DIR_LOG/alert $DIR_LOG/alert.bak
mv $DIR_LOG/proc_status $DIR_LOG/proc_status.bak
~/LiveExec/scripts/check_proc_status.pl > $DIR_LOG/proc_status
grep "^ALERT" $DIR_LOG/proc_status > $DIR_LOG/alert

diff -u $DIR_LOG/alert $DIR_LOG/alert.bak | grep "^+ALERT" > $DIR_LOG/recovery_mail
diff -u $DIR_LOG/alert $DIR_LOG/alert.bak | grep "^-ALERT" > $DIR_LOG/alert_mail

   
IS_ALERT_NONEMPTY=$(cat /spare/local/files/tmp/alert_mail | wc -c);
# There is an alert, send an email
if [ $IS_ALERT_NONEMPTY != 0 ]; then
DATE=$(date);
SUBJECT="ALERT : Processes on Trading servers $DATE";
EMAIL="nseall@tworoads.co.in";
/bin/mail -s "$SUBJECT" "$EMAIL" < /spare/local/files/tmp/alert_mail;
fi

IS_RECOVERY_NONEMPTY=$(cat /spare/local/files/tmp/recovery_mail | wc -c);
# There is an alert, send an email
if [ $IS_RECOVERY_NONEMPTY != 0 ]; then
DATE=$(date);
SUBJECT="RECOVERY : Processes on Trading servers $DATE";
EMAIL="nseall@tworoads.co.in";
/bin/mail -s "$SUBJECT" "$EMAIL" < /spare/local/files/tmp/recovery_mail;
fi
