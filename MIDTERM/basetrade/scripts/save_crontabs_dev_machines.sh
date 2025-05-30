#!/bin/bash

SINGLEMAC="na";
if [ $# -gt 0 ] ; then
SINGLEMAC=$1; shift;

YYYYMMDD=`date +%Y%m%d`;
CRONDIR=$HOME/crontabs;
mkdir -p $CRONDIR;

TRD_MAC=$SINGLEMAC;

    DATEDCRONFILE=$CRONDIR/crontab.$TRD_MAC.$YYYYMMDD;
    CRONFILE=$CRONDIR/crontab.$TRD_MAC;
#    echo $TRD_MAC
    if [ $USER == "sghosh" ] || [ $USER == "ravi" ] ;
    then
	ssh dvctrader@$TRD_MAC crontab -l > $DATEDCRONFILE ;
    else
	ssh $TRD_MAC crontab -l > $DATEDCRONFILE ;
    fi
    install -p --backup=t -T $DATEDCRONFILE $CRONFILE ;


find $CRONDIR -name *~ -type f -mtime +80 -exec rm -f {} \;

else

YYYYMMDD=`date +%Y%m%d`;
CRONDIR=$HOME/crontabs;
mkdir -p $CRONDIR;

for TRD_MAC in \
10.23.199.51 10.23.199.52 10.23.199.53 10.23.199.54 10.23.199.55 \
10.23.142.51
do 
    DATEDCRONFILE=$CRONDIR/crontab.$TRD_MAC.$YYYYMMDD;
    CRONFILE=$CRONDIR/crontab.$TRD_MAC;
#    echo $TRD_MAC
    if [ $USER == "sghosh" ] || [ $USER == "ravi" ];
    then
	ssh dvctrader@$TRD_MAC crontab -l > $DATEDCRONFILE ;
    else
	ssh $TRD_MAC crontab -l > $DATEDCRONFILE ;
    fi
    install -p --backup=t -T $DATEDCRONFILE $CRONFILE ;
done


find $CRONDIR -name \*~ -type f -mtime +80 -exec rm -f {} \;


fi
