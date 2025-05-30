#!/bin/bash

files13=`ssh 10.23.227.63 "ls /spare/local/MDSlogs/GENERIC/ | grep '_CE_'  | wc -l"`
files13_nif=`ssh 10.23.227.63 "ls /spare/local/MDSlogs/GENERIC_NIFTY/ | grep '_CE_'  | wc -l"`
ind13_files=$(( files13_nif + files13))

date_=`date +%Y%m%d`
echo "IND13 FILES $ind13_files"
i=0
echo `date`
while [ $i -lt 3 ];
do
	echo "SYNC CE from ind13"
	rsync -avz --progress --exclude="*_FUT_*" --exclude="*_PE_*" --exclude="*.gz"  10.23.115.63:/spare/local/MDSlogs/GENERIC_NIFTY/ /spare/local/MDSlogs/GENERIC
#	echo "SYNC 2"
#	rsync -avz --progress --include="*_CE_*" --exclude="*" 10.23.115.63:/spare/local/MDSlogs/GENERIC/ /spare/local/MDSlogs/GENERIC
#	echo "Sync 3"
#	rsync -avz --progress --include="*_CE_*" --exclude="*" 10.23.115.63:/spare/local/MDSlogs/GENERIC/ALLDATA_${date_}/ /spare/local/MDSlogs/GENERIC
	i=$((i+1))
done

files12=`ls /spare/local/MDSlogs/GENERIC/ | grep '_CE_' | grep ${date_} | wc -l`
echo "IND12 FILES $files12 IND13 $ind13_files"
echo `date`
if [[ $ind13_files -ne $files12 ]];then
echo ""| mailx -s "Problem with IND12 CE SYNC FROM IND13 COUNT13: $ind13_files COUNT12: $files12" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in
exit 1
else 
 echo ""| mailx -s "IND12 CE SYNC FROM IND13 COUNT13: $ind13_files COUNT12: $files12" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in
exit 0
fi

#echo "gzip files on ind13 GENERIC_NIFTY in back"
#bash; ssh 10.23.227.63  "cd /spare/local/MDSlogs/GENERIC_NIFTY; gzip *" & 
#ssh 10.23.227.63  "mv /spare/local/MDSlogs/GENERIC_NIFTY /spare/local/MDSlogs/GENERIC_NIFTY_${date_}"
#ssh 10.23.227.63  "mkdir /spare/local/MDSlogs/GENERIC_NIFTY"

echo `date`
echo "DONE "
