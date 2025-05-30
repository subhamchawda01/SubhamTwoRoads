#!/bin/bash

USAGE="$0 ID ";
if [ $# -ne 1 ] ; 
then 
    echo $USAGE;
    exit;
fi

STRATID=$1; shift;
STRATFILEPATH=`crontab -l | grep start_real | grep $STRATID | awk '{print $9}'`;
if [ -e $STRATFILEPATH ] ; then

~/basetrade/scripts/showParam.pl $STRATFILEPATH

fi