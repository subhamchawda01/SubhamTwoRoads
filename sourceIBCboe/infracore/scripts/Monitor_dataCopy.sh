#!/bin/bash
OutFile="/tmp/googleDocdatacopy"
>$OutFile
prevFile="/tmp/preDataCopyResults"
today=`date +"%Y%m%d"`
yyyy=${today:0:4}
mm=${today:4:2}
dd=${today:6:2}

#check COunt in ALLDATA
countALLDATA=`ssh dvcinfra@10.23.227.63 "ls /spare/local/MDSlogs/NSE/ | wc -l"`

#check Count IND13
countIND13=`ssh dvcinfra@10.23.227.63 "ls /spare/local/MDSlogs/${yyyy}/${mm}/${dd}/| wc -l"`

#check Syncing from IND13 to 67
sync13=`ssh dvcinfra@10.23.227.63 "ps aux | grep rsync |grep -v grep| grep 67| egrep \"NSELoggedData|MDSlogs\" | wc -l"`

#check count 67
count67=`ls /NAS1/data/NSELoggedData/NSE/${yyyy}/${mm}/${dd}/| wc -l`

#check Syncing from 67
sync67=`ps aux | grep rsync | grep -v grep| grep 63 | egrep "NSELoggedData|MDSlogs" | wc -l`

#check Count on Worker
countWorker=`ssh dvctrader@52.3.22.99 "ls /NAS1/data/NSELoggedData/NSE/$yyyy/$mm/$dd | wc -l"`

#check Sync to worker
syncworker=`ps aux | grep rsync | grep 148| grep -v grep | grep NSELoggedData | wc -l`

#check Jobs Running on Worker
jobWorker=`ssh dvctrader@52.3.22.99 'ps aux | grep /home/dvctrader/EOD_SCRIPTS/eod_jobs.sh | grep -v grep | wc -l'`

touch $prevFile
IFS=' '; read -r precountALLDATA precountIND13 presync13 precount67 presync67 precountWorker presyncworker prejobWorker<$prevFile

echo -e "\tMACHINE\tDIR\t\tCOUNT\tT-10Count" >> $OutFile
echo >>$OutFile
echo -e "\tIND13\tNSE\t\t$countALLDATA\t$precountALLDATA" >> $OutFile
echo -e "\tIND13\tMDSLOGS\t\t$countIND13\t$precountIND13" >> $OutFile
echo -e "\tIND13\tSyncTo67\t$sync13\t$presync13" >> $OutFile
echo -e "\t67\tNSELOGGEDDATA\t$count67\t$precount67" >> $OutFile
echo -e "\t67\tSyncfrom13\t$sync67\t$presync67" >> $OutFile
echo -e "\tWorker\tNSELOGGEDDATA\t$countWorker\t$precountWorker" >> $OutFile
echo -e "\tWorker\tSyncToWorker\t$syncworker\t$presyncworker" >> $OutFile
echo -e "\tWorker\tJobs\t\t$jobWorker\t$prejobWorker" >> $OutFile

echo "$countALLDATA $countIND13 $sync13 $count67 $sync67 $countWorker $syncworker $jobWorker" >$prevFile
