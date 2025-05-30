#!/bin/sh

if [ $# -lt 4 ] ; then
    echo "USAGE: $0 sub toaddr attfile txtfile [senderemail=user]";
    exit;
fi

SUBJECT=$1; 
TOADDR=$2; 
ATTFILENAME=$3; 
TXTFILENAME=$4;
SENDEREMAIL=$USER@circulumvite.com
if [ $# -gt 4 ] ; then
    SENDEREMAIL=$5;
fi

#echo mailx -s $SUBJECT -r$SENDEREMAIL $TOADDR from $TXTFILENAME with attachment $ATTFILENAME
mailx -s $SUBJECT -a $ATTFILENAME -r$SENDEREMAIL $TOADDR < $TXTFILENAME ;
