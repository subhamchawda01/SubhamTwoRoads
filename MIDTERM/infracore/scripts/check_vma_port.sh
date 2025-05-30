#!/bin/bash

IBDEVOUT=`ibdev2netdev`
HOSTNAME=`hostname`

MAIL_FILE=/tmp/vma_port_check_mail_file.txt
>$MAIL_FILE;

if [[ $IBDEVOUT != *eth7* ]]
then
    MAILBODY="vma port is down in $HOSTNAME."
    echo $MAILBODY

    echo "vma port is down in $HOSTNAME" >> $MAIL_FILE;

    #/bin/mail -s "VMA-PORT-DOWN ALERT" -r "vma_port_check@$HOSTNAME" "hardik@tworoads.co.in" < $MAIL_FILE
    /bin/mail -s "VMA-PORT-DOWN ALERT" -r "vma_port_check@$HOSTNAME" "nseall@tworoads.co.in" < $MAIL_FILE
fi
