#!/bin/bash
USAGE="$0  YYYYMMDD";
if [ $# -ne 1 ] ;
then
    echo $USAGE
    exit -1;
fi
date=$1
if [ $1 == "TODAY" ]; then
        date=`date +"%Y%m%d"`;
fi
today=$date

isRatioGenTmp='/tmp/numRatioGen.txt'
calcRatioTmp='/tmp/calcratiops.txt'
killedInsTmp='/tmp/killedrationum.txt'
psTmp='/tmp/numratioprocessrunning.txt'
tmpfiles='/tmp/tmpfiles.txt'

grep "calc_ratio" /home/dvctrader/EOD_SCRIPTS/eod_jobs.sh > calcRatioTmp
findRatioFilesToCompare () {
   filepath=$2
   directoryToSearch=$1
   cd /spare/local/NseHftFiles/Ratio/"$directoryToSearch"
   i=`echo $filepath | egrep "FUT1_RATIO" | wc -l`
   if [[ $i -eq 1 ]];then
     ls | egrep 'FUT1.*FUT0$' >tmpfiles
   fi
   i=`echo $filepath | egrep "FUT2_RATIO" | wc -l`
   if [[ $i -eq 1 ]];then
     ls | egrep 'FUT2.*FUT1$' >tmpfiles
   fi
   i=`echo $filepath | egrep "NSE_CM.*FUT1" | wc -l`
   if [[ $i -eq 1 ]];then
     ls | egrep -v 'FUT0$|FUT.*FUT1$' | egrep "FUT1$" >tmpfiles
   fi
   i=`echo $filepath | egrep "NSE_CM" | egrep -v "FUT1" | wc -l`
   if [[ $i -eq 1 ]];then
     ls | egrep -v 'FUT1$|FUT.*FUT0$' | egrep "FUT0$" >tmpfiles
   fi
   return
}
while read -r calc_ratio;
do
   cat `echo $calc_ratio | awk '{sub(/1>/,"",$8)}{print $8}'` | grep -i killed | wc -l >killedInsTmp
   killedInstance=`cat killedInsTmp`
   if [[ $killedInstance -ne 0 ]]; then
       cd /spare/local/NseHftFiles/Ratio/`echo $calc_ratio | awk '{print $6}'`
       findRatioFilesToCompare `echo $calc_ratio | awk '{print $6}'` `echo $calc_ratio | awk '{print $7}'`
       egrep "$date" -rnw `cat tmpfiles` |wc -l > isRatioGenTmp
       ratioInstances=`cat isRatioGenTmp`
       if [[ $ratioInstances -eq 0 ]]; then
           ps aux | grep `echo $calc_ratio | awk '{print $1}'` | grep `echo "$calc_ratio" | awk '{print $3}'` |wc -l >psTmp
           psInstance=`cat psTmp`
           if [[ $psInstance -eq 0 ]]; then
             $calc_ratio
           fi
       fi
   fi
done<calcRatioTmp





