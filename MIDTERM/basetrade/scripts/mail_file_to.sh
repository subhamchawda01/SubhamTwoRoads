#!/bin/sh

if [ $# -lt 3 ] ; then
    echo "USAGE: $0 sub toaddr file [senderemail=user]";
    exit;
fi

SUBJECT=$1; 
TOADDR=$2; 
FILENAME=$3; 
SENDEREMAIL=$USER@circulumvite.com
if [ $# -gt 3 ] ; then
    SENDEREMAIL=$4;
fi

#echo mailx -s $SUBJECT -r$SENDEREMAIL $TOADDR from $FILENAME
mailx -s $SUBJECT -r$SENDEREMAIL $TOADDR < $FILENAME
