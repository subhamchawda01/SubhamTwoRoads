#!/bin/bash

FTP_HOST='ftp.connect2nse.com'

dowload_price_band() {
ftp -np $FTP_HOST <<SCRIPT
  user "ftpguest" "FTPGUEST" 
  cd Common
  cd NTNEAT
  binary 
  get security.gz
  quit 
SCRIPT

gunzip -f security.gz
}

GetPreviousWorkingDay() {
  YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P W`
  is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  while [ $is_holiday_ = "1" ];
  do
    $YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P W`
    is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  done
}


HHMM=`date +"%H%M"`
YYYYMMDD=`date +"%Y%m%d"`
if [ ${HHMM} -lt 1000 ];
then
  GetPreviousWorkingDay;
fi
DD=${YYYYMMDD:6:2}
MM=${YYYYMMDD:4:2}
YYYY=${YYYYMMDD:0:4};
#update YYYYMMDD to previous working day
GetPreviousWorkingDay;

FTP_DIR="/spare/local/files/NSEFTPFiles";
cd ${FTP_DIR}"/"${YYYYMMDD}"/"${MM}"/"${DD}"/";
diff_count=`diff -u ${FTP_DIR}"/"${YYYYMMDD:0:4}"/"${YYYYMMDD:4:2}"/"${YYYYMMDD:6:2}/security \
           ${FTP_DIR}"/"${YYYY}"/"${MM}"/"${DD}/security | wc -l`

#simply exit if the file is already updated
[ $diff_count -ne 0 ] && exit;
[ $diff_count -eq 0 ] && dowload_price_band;

diff_count=`diff -u ${FTP_DIR}"/"${YYYYMMDD:0:4}"/"${YYYYMMDD:4:2}"/"${YYYYMMDD:6:2}/security \
           ${FTP_DIR}"/"${YYYY}"/"${MM}"/"${DD}/security | wc -l`
[ ${diff_count} -ne 0 ] && echo "" | mailx -s "PRICE BAND FILE UPDATED" -r sanjeev.kumar@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in

#should get updated in next run
[ ${diff_count} -eq 0 ] && exit;
rsync -ravz --timeout=60 -e "ssh -p 22761" /spare/local/files/NSEFTPFiles 202.189.245.205:/spare/local/files --delete-after
rsync -ravz --timeout=60 -e "ssh -p 22763" /spare/local/files/NSEFTPFiles 202.189.245.205:/spare/local/files --delete-after
rsync -ravz --timeout=60 -e "ssh -p 22764" /spare/local/files/NSEFTPFiles 202.189.245.205:/spare/local/files --delete-after
rsync -ravz --timeout=60 -e "ssh -p 22765" /spare/local/files/NSEFTPFiles 202.189.245.205:/spare/local/files --delete-after
rsync -ravz --timeout=60 -e "ssh -p 22781" /spare/local/files/NSEFTPFiles 202.189.245.205:/spare/local/files --delete-after
rsync -ravz --timeout=60 -e "ssh -p 22782" /spare/local/files/NSEFTPFiles 202.189.245.205:/spare/local/files --delete-after
rsync -ravz --timeout=60 -e "ssh -p 22783" /spare/local/files/NSEFTPFiles 202.189.245.205:/spare/local/files --delete-after
rsync -ravz --timeout=60 -e "ssh -p 22769" /spare/local/files/NSEFTPFiles 202.189.245.205:/spare/local/files --delete-after
rsync -ravz --timeout=60 -e "ssh -p 22784" /spare/local/files/NSEFTPFiles 202.189.245.205:/spare/local/files --delete-after
rsync -ravz /spare/local/files/NSEFTPFiles 10.23.227.83:/spare/local/files --delete-after
rsync -ravz /spare/local/files/NSEFTPFiles 10.23.227.84:/spare/local/files --delete-after
rsync -ravz /spare/local/files/NSEFTPFiles 10.23.227.63:/spare/local/files --delete-after
rsync -ravz /spare/local/files/NSEFTPFiles 10.23.227.64:/spare/local/files --delete-after
rsync -ravz /spare/local/files/NSEFTPFiles 10.23.227.65:/spare/local/files --delete-after
rsync -ravz /spare/local/files/NSEFTPFiles 10.23.227.69:/spare/local/files --delete-after
rsync -ravz /spare/local/files/NSEFTPFiles 10.23.227.81:/spare/local/files --delete-after
rsync -ravz /spare/local/files/NSEFTPFiles 10.23.227.82:/spare/local/files --delete-after
rsync -avz /spare/local/files/NSEFTPFiles 3.89.148.73:/spare/local/files
