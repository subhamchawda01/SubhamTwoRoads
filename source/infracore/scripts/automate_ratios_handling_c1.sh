#!/bin/bash

isRatioGenTmp='/tmp/numRatioGen.txt'
calcRatioTmp='/tmp/calcratiops.txt'
killedInsTmp='/tmp/killedrationum.txt'
psTmp='/tmp/numratioprocessrunning.txt'
tmpfiles='/tmp/tmpfiles_112.txt'

findRatioFilesToCompare () {
   filepath=$live_file
   echo "Path To Search "$ratio_path
   cd $ratio_path
   i=`echo $filepath | egrep "FUT1_RATIO" | wc -l`
   if [[ $i -eq 1 ]];then
     ls | egrep 'FUT1.*FUT0$' >$tmpfiles
   fi
   i=`echo $filepath | egrep "FUT2_RATIO" | wc -l`
   if [[ $i -eq 1 ]];then
     ls | egrep 'FUT2.*FUT1$' >$tmpfiles
   fi
   i=`echo $filepath | egrep "NSE_CM.*FUT1" | wc -l`
   if [[ $i -eq 1 ]];then
     ls | egrep -v 'FUT0$|FUT.*FUT1$' | egrep "FUT1$" >$tmpfiles
   fi
   i=`echo $filepath | egrep "NSE_CM" | egrep -v "FUT1" | wc -l`
   if [[ $i -eq 1 ]];then
     ls | egrep -v 'FUT1$|FUT.*FUT0$' | egrep "FUT0$" >$tmpfiles
   fi
   return
}


USAGE="$0  YYYYMMDD";
if [ $# -ne 1 ] ;
then
    echo $USAGE
    exit -1;
fi
today_=$1
if [ $1 == "TODAY" ]; then
        today_=`date +"%Y%m%d"`;
fi
echo "TODAY $today_"
start_eod=`grep "Start: $today_" /spare/local/files/eod_complete.txt| wc -l`

if [[ $start_eod -eq 0 ]]; then
		echo "EOD NOT STARTED: Existing";
		exit
fi

exit
grep "calc_ratio" /home/dvctrader/EOD_SCRIPTS/eod_jobs.sh > $calcRatioTmp

while read -r calc_ratio;
do
   killedInstance=`echo $calc_ratio | awk '{sub(/1>/,"",$8)}{print $8}'| xargs cat | grep -i killed | wc -l`
   id_=`echo $calc_ratio | awk '{sub(/1>/,"",$8)}{print $3}'`
   type_ratio=`echo $calc_ratio | awk '{print $6}'`
   live_file=`echo $calc_ratio | awk '{print $7}'`
   echo "ID: $id_ TYPE: $type_ratio KilledIn: $killedInstance LIVE: $live_file"
   ratio_path="/spare/local/NseHftFiles/Ratio/$type_ratio"
   if [[ $killedInstance -ne 0 ]]; then
       
       mkdir -p $ratio_path
       findRatioFilesToCompare
       cd $ratio_path

       egrep "$today_" -rnw `cat $tmpfiles` |wc -l > $isRatioGenTmp
       ratioInstances=`cat $isRatioGenTmp`
       echo "Ratio Instances: $ratioInstances"
       if [[ $ratioInstances -eq 0 ]]; then
           psInstance=`ps aux | grep 'scripts/calc_ratio.sh' | grep $id_ | grep -v grep | wc -l`
	   echo "ps aux | grep 'scripts/calc_ratio.sh' | grep $id_ | grep -v grep | wc -l"
	   echo "Running instance $psInstance"
           if [[ $psInstance -eq 0 ]]; then
             echo "Running Ratio Now $calc_ratio"
	     $calc_ratio
           fi
       fi
   fi
done< $calcRatioTmp

