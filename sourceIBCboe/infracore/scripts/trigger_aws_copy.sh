#!/bin/bash 

#First Fetch the datacopy files from AWS 

rsync -avz dvcinfra@10.23.74.40:/apps/data/AWSTrigger /home/dvctrader >/dev/null 2>/dev/null
rsync -avz dvcinfra@10.23.74.51:/apps/data/AWSTrigger /home/dvctrader >/dev/null 2>/dev/null
rsync -avz /apps/data/MICEXLoggedData dvcinfra@10.23.74.40:/apps/data/>/dev/null 2>/dev/null
rsync -avz /apps/data/RTSLoggedData dvcinfra@10.23.74.40:/apps/data/>/dev/null 2>/dev/null
rsync -avz /apps/data/OSELoggedData dvcinfra@10.23.74.40:/apps/data/>/dev/null 2>/dev/null
rsync -avz /apps/data/OSEPriceFeedLoggedData dvcinfra@10.23.74.40:/apps/data/>/dev/null 2>/dev/null
rsync -avz /apps/data/OSENewPriceFeedLoggedData dvcinfra@10.23.74.40:/apps/data/>/dev/null 2>/dev/null
rsync -avz /apps/data/OSE_L1LoggedData dvcinfra@10.23.74.40:/apps/data/>/dev/null 2>/dev/null
rsync -avz /apps/data/HKEXLoggedData dvcinfra@10.23.74.40:/apps/data/>/dev/null 2>/dev/null
rsync -avz /apps/data/EUREXLoggedData dvcinfra@10.23.74.40:/apps/data/>/dev/null 2>/dev/null
rsync -avz /apps/data/EOBIPriceFeedLoggedData dvcinfra@10.23.74.40:/apps/data/>/dev/null 2>/dev/null
rsync -avz /apps/data/CMELoggedData dvcinfra@10.23.74.40:/apps/data/>/dev/null 2>/dev/null
rsync -avz /apps/logs/QueryTrades/ dvcinfra@10.23.74.40:/apps/logs/QueryTrades/>/dev/null 2>/dev/null
rsync -avz /apps/logs/QueryLogs/ dvcinfra@10.23.74.40:/apps/logs/QueryLogs/>/dev/null 2>/dev/null
rsync -avz /apps/data/OSELoggedData dvcinfra@10.23.74.40:/apps/data/>/dev/null 2>/dev/null
rsync -avz /apps/data/OSEPriceFeedLoggedData dvcinfra@10.23.74.40:/apps/data/>/dev/null 2>/dev/null
rsync -avz /apps/data/OSENewPriceFeedLoggedData dvcinfra@10.23.74.40:/apps/data/>/dev/null 2>/dev/null
rsync -avz /apps/data/OSE_L1LoggedData dvcinfra@10.23.74.40:/apps/data/>/dev/null 2>/dev/null
rsync -avz /apps/data/HKEXLoggedData dvcinfra@10.23.74.40:/apps/data/>/dev/null 2>/dev/null
rsync -avz /apps/data/EUREXLoggedData dvcinfra@10.23.74.40:/apps/data/>/dev/null 2>/dev/null
rsync -avz /apps/data/EOBIPriceFeedLoggedData dvcinfra@10.23.74.40:/apps/data/>/dev/null 2>/dev/null
rsync -avz /apps/data/CONTROLLoggedData dvcinfra@10.23.74.40:/apps/data/>/dev/null 2>/dev/null
rsync -avz /apps/data/CMELoggedData dvcinfra@10.23.74.40:/apps/data/>/dev/null 2>/dev/null
rsync -avz /apps/logs/QueryTrades dvcinfra@10.23.74.40:/apps/logs>/dev/null 2>/dev/null
rsync -avz /apps/logs/QueryLogs dvcinfra@10.23.74.40:/apps/logs>/dev/null 2>/dev/null
rsync -avz /apps/data/ORSData/HK dvcinfra@10.23.74.40:/apps/data/ORSData>/dev/null 2>/dev/null
rsync -avz /apps/data/ORSData/TOK dvcinfra@10.23.74.40:/apps/data/ORSData>/dev/null 2>/dev/null
rsync -avz /apps/logs/ORSTrades/HKEX dvcinfra@10.23.74.40:/apps/logs/ORSTrades>/dev/null 2>/dev/null
rsync -avz /apps/data/EUREXLoggedData/MOS dvcinfra@10.23.74.40:/apps/data/EUREXLoggedData>/dev/null 2>/dev/null
rsync -avz /apps/data/ICELoggedData/MOS dvcinfra@10.23.74.40:/apps/data/ICELoggedData>/dev/null 2>/dev/null
rsync -avz /apps/data/EOBIPriceFeedLoggedData/MOS dvcinfra@10.23.74.40:/apps/data/EOBIPriceFeedLoggedData>/dev/null 2>/dev/null
rsync -avz /apps/data/CMELoggedData/MOS dvcinfra@10.23.74.40:/apps/data/CMELoggedData>/dev/null 2>/dev/null
rsync -avz /apps/data/QUINCY3LoggedData/MOS dvcinfra@10.23.74.40:/apps/data/QUINCY3LoggedData>/dev/null 2>/dev/null
rsync -avz /apps/data/LIFFELoggedData/MOS dvcinfra@10.23.74.40:/apps/data/LIFFELoggedData>/dev/null 2>/dev/null
rsync -avz /apps/data/RTSLoggedData/MOS dvcinfra@10.23.74.40:/apps/data/RTSLoggedData>/dev/null 2>/dev/null
rsync -avz /apps/data/RTSCombinedLoggedData/MOS dvcinfra@10.23.74.40:/apps/data/RTSCombinedLoggedData>/dev/null 2>/dev/null
rsync -avz /apps/data/RTS_P2LoggedData/MOS dvcinfra@10.23.74.40:/apps/data/RTS_P2LoggedData>/dev/null 2>/dev/null
rsync -avz /apps/data/MICEXLoggedData/MOS dvcinfra@10.23.74.40:/apps/data/MICEXLoggedData>/dev/null 2>/dev/null
rsync -avz /apps/data/EBSLoggedData/MOS dvcinfra@10.23.74.40:/apps/data/EBSLoggedData>/dev/null 2>/dev/null
rsync -avz /apps/logs/QueryTrades dvcinfra@10.23.74.40:/apps/logs>/dev/null 2>/dev/null
rsync -avz /apps/logs/QueryLogs dvcinfra@10.23.74.40:/apps/logs>/dev/null 2>/dev/null
rsync -avz /apps/data/ORSData/MOS dvcinfra@10.23.74.40:/apps/data/ORSData>/dev/null 2>/dev/null
rsync -avz /apps/logs/ORSLogs/RTS dvcinfra@10.23.74.40:/apps/logs/ORSLogs>/dev/null 2>/dev/null
rsync -avz /apps/logs/ORSLogs/RTSDC dvcinfra@10.23.74.40:/apps/logs/ORSLogs>/dev/null 2>/dev/null
rsync -avz /apps/logs/ORSTrades/RTS dvcinfra@10.23.74.40:/apps/logs/ORSTrades>/dev/null 2>/dev/null
rsync -avz /apps/logs/ORSTrades/RTSDC dvcinfra@10.23.74.40:/apps/logs/ORSTrades>/dev/null 2>/dev/null
rsync -avz /apps/logs/ORSLogs/MICEX dvcinfra@10.23.74.40:/apps/logs/ORSLogs>/dev/null 2>/dev/null
rsync -avz /apps/logs/ORSLogs/MICEXDC dvcinfra@10.23.74.40:/apps/logs/ORSLogs>/dev/null 2>/dev/null
rsync -avz /apps/logs/ORSTrades/MICEX dvcinfra@10.23.74.40:/apps/logs/ORSTrades>/dev/null 2>/dev/null
rsync -avz /apps/logs/ORSTrades/MICEXDC dvcinfra@10.23.74.40:/apps/logs/ORSTrades>/dev/null 2>/dev/null

TODAY=`date +"%Y%m%d"`
AWS_TRIGGER_COPY_DIR=/home/dvctrader/AWSTrigger 
RUNFILE=/apps/data/AWSTrigger/.run 
TEMP_COPY_LISTFILE=/tmp/temp_data.tmp 

#Make Sure This File is Not Deleted Unintentionally 
if [ ! -f $RUNFILE ] 
then 

  echo "Run File Not Available, AWS Copy Trigger" | /bin/mail -s "AWS Trigger" -r "awstriggercopy@ny11" "nseall@tworoads.co.in"; 
#  echo "Run File Not Available, AWS Copy Trigger" | /bin/mail -s "AWS Trigger" -r "awstriggercopy@ny11" "ravi.parikh@tworoads.co.in"; 
  touch $RUNFILE ; 
  chmod 000 $RUNFILE ; # No permissions on this file, only inode permissions 

fi 

>$TEMP_COPY_LISTFILE ;

for file in `find $AWS_TRIGGER_COPY_DIR -type f -newer $RUNFILE`
do

  cat $file >> $TEMP_COPY_LISTFILE ; 

done 

#Immediate Call to Touch makes sure that delay due to upload is not included 
touch $RUNFILE ; 

echo "Date : " `date` ; echo ; 
cat $TEMP_COPY_LISTFILE ;
 
#cat $TEMP_COPY_LISTFILE | while read a ; do /apps/s3cmd/s3cmd-1.5.0-alpha1/s3cmd put $a s3://s3dvc$a ; done 

for file in `cat $TEMP_COPY_LISTFILE`
do

  echo "/apps/s3cmd/s3cmd-1.5.0-alpha1/s3cmd put $file s3://s3dvc$file ;" 
  /apps/s3cmd/s3cmd-1.5.0-alpha1/s3cmd put $file s3://s3dvc$file ; 

  #Upload to HS1 server (EC2 file host) as well
  hs1_disk=`$HOME/LiveExec/bin/get_hs1_path $file`;
  rsync -aR $file dvctrader@52.0.55.252:$hs1_disk/s3_cache/;

done 

rm -rf $TEMP_COPY_LISTFILE ;

