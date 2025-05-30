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
10.23.74.61 \
10.23.200.51 10.23.200.52 10.23.200.53 10.23.200.54 \
10.23.82.51 10.23.82.52 10.23.82.53 10.23.82.54 \
10.23.182.51 10.23.182.52 \
10.23.23.11 10.23.23.12 10.23.23.13 10.23.23.14 \
10.23.52.51 10.23.52.52 10.23.52.53 \
10.152.224.145 10.152.224.146 \
10.134.210.184 \
172.18.244.107 \
10.23.74.51 10.23.74.52 10.23.74.53 10.23.74.54 10.23.74.55
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
    for file in .bashrc .bash_profile;
    do
      file_path=$CRONDIR/setting.$file.$TRD_MAC.$YYYYMMDD;
      scp $TRD_MAC:$file $file_path;
    done
done


find $CRONDIR -name \*~ -type f -mtime +80 -exec rm -f {} \;


fi
