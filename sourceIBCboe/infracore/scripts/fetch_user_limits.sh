#!/bin/bash
YYYYMMDD=`date +"%Y%m%d"`
DDMMYYYY=`date +"%d%m%Y"`
FTP_HOST='ftp.connect2nse.com'
MEMBER_ID='90044'
FO_USER_ID='F90044'
CM_USER_ID='90044'
FO_PASSWD='May@2019'
CM_PASSWD='May@2019'

USERLIMITSREPORTSDIR='/spare/local/tradeinfo/NSE_Files/UserLimits';

IS_HOLIDAY=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
if [ $IS_HOLIDAY = "1" ];then
        echo "NSE holiday..., exiting";
        exit
fi

cd ${USERLIMITSREPORTSDIR}

#download for fo
ftp -n $FTP_HOST <<SCRIPT
	user ${FO_USER_ID} ${FO_PASSWD}
	cd faoftp
	cd F90044
	cd Reports
	cd Order_Reports
	binary
	get FO_UL_${MEMBER_ID}_${DDMMYYYY}1.csv.gz
SCRIPT

#download for cm
ftp -n $FTP_HOST <<SCRIPT
	user ${CM_USER_ID} ${CM_PASSWD}
	cd 90044
	cd REPORTS
	cd Order_Reports
	binary
	get CM_UL_${MEMBER_ID}_${DDMMYYYY}1.csv.gz
SCRIPT
if [ ! -f CM_UL_${MEMBER_ID}_${DDMMYYYY}1.csv.gz ] || [ ! -f FO_UL_${MEMBER_ID}_${DDMMYYYY}1.csv.gz ];
then
	echo "" | mailx -s "FAILED DOWLOADING USER LIMITS FILES" -r "${HOSTNAME}-${USER}<sanjeev.kumar@tworoads-trading.co.in>" sanjeev.kumar@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in
	exit;
fi

gunzip -f CM_UL_${MEMBER_ID}_${DDMMYYYY}1.csv.gz
gunzip -f FO_UL_${MEMBER_ID}_${DDMMYYYY}1.csv.gz 
#sync to all prod machines 
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.62:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.61:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.63:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.64:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.65:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.69:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.81:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.82:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.83:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.84:/spare/local/tradeinfo --delete-after
rsync -avz --progress /spare/local/tradeinfo/NSE_Files 3.89.148.73:/spare/local/tradeinfo

