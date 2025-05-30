#!/bin/bash

#args -> pairsfile

#creating tranches
echo 'creating tranches...'
filename=`echo $1 | awk -F '/' '{print $(NF)}'`

mkdir -p $HOME/SpreadTraderParams/Tranches/$filename
mkdir -p $HOME/SpreadTraderParams/master
mkdir -p $HOME/SpreadTraderParams/params
cd $HOME/SpreadTraderParams/Tranches/$filename
for f in `cat $1 | awk '{split($0,a,"_");print a[3]}' | sort -u`; do rm $f; done;
cat $1 | grep -v HDIL| grep -v IBREALEST|grep -v HDIL | tr '[&]' '~'| awk '{split($0,a,"_"); print a[1]"\t"a[2]>>a[3]}'


#creating params
ID=`echo $filename| awk -F '_' '{print $(NF)}'`

#remove params having same ID
for f in `ls $HOME/SpreadTraderParams/params/*_"$ID"_*_*`; do echo "deleting "$f; rm $f; done;

#remove masterparams having same ID
for f in `ls $HOME/SpreadTraderParams/master/masterparam_"$ID"_*_*`; do echo "deleting "$f; rm $f; done;

for i in {1..12}; do
    echo 'creating params for duration of '$i' month...'
    for f in `ls`;do bash $HOME/basetrade/hftrap/SpreadTrading/bulk_param.sh $ID"_"$i"_"$f $f; done;
done;        

#running spread_exec    
#echo 'running spread_exec...'

cd $HOME/basetrade

MAXJOBS=15;

PARAMCOUNT=`ls $HOME/SpreadTraderParams/master/masterparam_"$ID"_*_*|sort -u|wc -l`;
i=0;

for f in `ls $HOME/SpreadTraderParams/master/masterparam_"$ID"_*_*`; do
    TSDATE=`echo $f| awk -F '_' '{print $(NF)}'`;
    Period=`echo $f| awk -F '_' '{print $(NF-1)}'`;
    SDATE=$(date +%Y%m%d -d "$TSDATE - 1 month");
    EDATE=$(date +%Y%m%d -d "$TSDATE + $Period month");
    EDATE=$(date +%Y%m%d -d "$EDATE - 1 day");
    ((i++));
    echo $i"/"$PARAMCOUNT ":Running $f --start_date $SDATE --end_date $EDATE --trade_start_date $TSDATE";
    $HOME/basetrade_install/bin/spread_exec --paramfile $f --start_date $SDATE --end_date $EDATE --trade_start_date $TSDATE --multiparam &
    
    while true; do
        NUMJOBS=`ps --ppid $$ -o pid= | wc | awk -F ' ' '{ print $1;}'`
        test $NUMJOBS -lt $MAXJOBS && break;
    done;
done;

