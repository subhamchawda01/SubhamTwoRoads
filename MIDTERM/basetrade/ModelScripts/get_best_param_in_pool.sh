#!/bin/bash

USAGE="<script> shc tp_dir strattype regresstype start_date end_date [skip_file=INVALIDFILE]";
if [ $# -lt 6 ]; then
	echo $USAGE;
	exit;
fi
shc=$1; 
tp_dir=$2;
strattype=$3;
regresstype=$4
start_date=$5;
end_date=$6;
skip_file="INVALIDFILE";
if [ $# -ge 7 ]; then
    skip_file=$7;
fi
uid=`date +%N`;
stratsfile=strats_"$uid";
tmpfile=tmp_"$uid";
paramsfile=params_"$uid";
results_dir="/NAS1/ec2_globalresults/";
#if [ `hostname` == "sdv-ny4-srv11.dvcap.local" ]; then results_dir="$HOME/ec2_globalresults/"; fi;
$HOME/basetrade_install/bin/summarize_strategy_results $shc $tp_dir $results_dir  $start_date $end_date $skip_file kCNAPnlAdjAverage 0 INVALIDFILE 0 | grep STRAT | awk '{print $2}' > $stratsfile;
if [ $regresstype = "LINEAR" ]; then grep -v "SIGLR\|EARTH" $stratsfile > $tmpfile; mv $tmpfile $stratsfile; fi;
if [ $regresstype = "SIGLR" ]; then grep SIGLR $stratsfile > $tmpfile; mv $tmpfile $stratsfile; fi;
for strat in `cat $stratsfile`; do if [ `cat $tp_dir/$strat | awk '{print $3}'` = $strattype ]; then echo $strat; fi ; done > $tmpfile; cat $tmpfile | head -n20 > $stratsfile;
if [ -s $stratsfile ]; then
    for strat in `cat $stratsfile`; do cat $tp_dir/$strat | awk '{print $5}'; done | sort | uniq -c | sort -n -r | head -n2 > $paramsfile; 
    while read line ; do param=`echo $line | awk '{print $2}'`; for strat in `cat $stratsfile`; do if [ `cat $tp_dir/$strat | awk '{print $5}'` = $param ]; then echo $line $tp_dir/$strat $shc $regresstype $strattype ; break; fi; done; done < $paramsfile;
fi
rm -f $stratsfile;
rm -f $tmpfile;
rm -f $paramsfile;
