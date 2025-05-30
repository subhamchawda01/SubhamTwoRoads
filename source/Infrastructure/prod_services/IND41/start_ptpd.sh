#!/bin/bash

# Log file location
LOGFILE="/var/log/ptpd2.log"

echo "Sleeping for 5 seconds"
sleep 5

echo "Clear /etc/resolve.conf file"
>/etc/resolve.conf

echo "Starting PTPd at $(date)"
echo "Log File: $LOGFILE"

/usr/local/sbin/ptpd2 -c /etc/sysconfig/ptpd2
