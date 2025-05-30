#!/bin/bash

USAGE1="$0 EPOCHTIME STZ";
USAGE2="$0 NOW STZ";

if [ $# -ne 2 ] ; 
then 
    echo $USAGE1;
    echo $USAGE2;
    exit;
fi

REPO=infracore;
GENSCRIPTSDIR=$HOME/$REPO/scripts;

EPOCHTIME=$1;
STZ=$2

if [ $EPOCHTIME = "NOW" ] ;
then
    EPOCHTIME=`date +"%s"` ;
fi

if [ $STZ = "TOR" ] ;
then
    env TZ=America/Toronto $GENSCRIPTSDIR/unixtime2localstr.pl $EPOCHTIME 1
elif [ $STZ = "NY" ] ;
then
    env TZ=America/New_York $GENSCRIPTSDIR/unixtime2localstr.pl $EPOCHTIME 1
elif [ $STZ = "CHI" ] ;
then
    env TZ=America/Chicago $GENSCRIPTSDIR/unixtime2localstr.pl $EPOCHTIME 1
elif [ $STZ = "BRZ" ] ;
then
    env TZ=America/Sao_Paulo $GENSCRIPTSDIR/unixtime2localstr.pl $EPOCHTIME 1
elif [ $STZ = "BSL" ] ;
then
    env TZ=Europe/London $GENSCRIPTSDIR/unixtime2localstr.pl $EPOCHTIME 1
elif [ $STZ = "FR" ] ;
then
    env TZ=Europe/Berlin $GENSCRIPTSDIR/unixtime2localstr.pl $EPOCHTIME 1
elif [ $STZ = "IND" ] ;
then
    env TZ=Asia/Kolkata $GENSCRIPTSDIR/unixtime2localstr.pl $EPOCHTIME 1
elif [ $STZ = "HK" ] ;
then
    env TZ=Asia/Hong_Kong $GENSCRIPTSDIR/unixtime2localstr.pl $EPOCHTIME 1
elif [ $STZ = "TOK" ] ;
then
    env TZ=Asia/Tokyo $GENSCRIPTSDIR/unixtime2localstr.pl $EPOCHTIME 1
elif [ $STZ = "UTC" ] ;
then
    env TZ="GST+0" $GENSCRIPTSDIR/unixtime2localstr.pl $EPOCHTIME 1
else
    echo "I dont know "
fi
    
