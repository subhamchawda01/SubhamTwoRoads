#!/bin/bash 

FTP_HOST='ftp.connect2nse.com'

get_expiry_date () {
  ONE=01;
  EXPIRY=$YYYYMMDD;
  for i in {1..7}; do dateStr=`date -d "$YYYY$MM$ONE + 1 month - $i day" +"%w %Y%m%d"`; if [ ${dateStr:0:1} -eq 4 ] ; then EXPIRY=${dateStr:2}; fi ; done
}

fetch_cd_ref_contracts () {
  ftp -np $FTP_HOST <<SCRIPT 
  user "CDSGUEST" "CDSGUEST"
  cd cdsftp
  cd cdscommon
  binary 
  get cd_contract.gz
  get cd_participant.gz
  get cd_spd_contract.gz
  quit
SCRIPT
gunzip -f cd_contract.gz
gunzip -f cd_participant.gz
gunzip -f cd_spd_contract.gz
} 

download_contract() {
  ftp -np $FTP_HOST <<SCRIPT
    user "FAOGUEST" "FAOGUEST"
    cd faoftp
    cd faocommon
    binary
    get contract.gz
    get fo_participant.gz
    get spd_contract.gz
    quit
SCRIPT
gunzip -f contract.gz
gunzip -f fo_participant.gz
gunzip -f spd_contract.gz
}

GetPreviousWorkingDay() {
  YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P W`
  is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  while [ $is_holiday_ = "1" ];
  do
    YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P W`
    is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  done
}

HHMM=`date +"%H%M"`
YYYYMMDD=`date +"%Y%m%d"`
if [ ${HHMM} -lt 1900 ];
then
  GetPreviousWorkingDay;
fi
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi
DD=${YYYYMMDD:6:2}
MM=${YYYYMMDD:4:2}
YYYY=${YYYYMMDD:0:4};
YY=${YYYYMMDD:2:2};
#update YYYYMMDD to previous working day
next_working_day=`/home/pengine/prod/live_execs/update_date $YYYY$MM$DD N W`
isholiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
while [ $is_holiday = "1" ] 
do
     next_working_day=`/home/pengine/prod/live_execs/update_date $next_working_day N W`
     is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
done
echo $next_working_day
GetPreviousWorkingDay;

FTP_DIR="/spare/local/files/NSEFTPFiles";
[[ $1 == "FORCE" ]] && `rm -rf "${FTP_DIR}"/"${YYYY}"/"${MM}"/"${DD}/contract" "${FTP_DIR}/${YYYY}/${MM}/${DD}/cd_contract"`
cd ${FTP_DIR}"/"${YYYY}"/"${MM}"/"${DD}"/";
diff_count=`diff -u ${FTP_DIR}"/"${YYYYMMDD:0:4}"/"${YYYYMMDD:4:2}"/"${YYYYMMDD:6:2}/contract \
           ${FTP_DIR}"/"${YYYY}"/"${MM}"/"${DD}/contract | wc -l`
diff_count2=`diff -u ${FTP_DIR}"/"${YYYYMMDD:0:4}"/"${YYYYMMDD:4:2}"/"${YYYYMMDD:6:2}/cd_contract \
            ${FTP_DIR}"/"${YYYY}"/"${MM}"/"${DD}/cd_contract | wc -l`
#simply exit if the file is already updated
[[ $diff_count -ne 0 ]] && [[ $diff_count2 -ne 0 ]] && { echo "CONTRACT FILE EXIST AND UPDATED ALREADY"; exit;}

[[ $diff_count -eq 0 ]] && download_contract;
[[ $diff_count2 -eq 0 ]] && fetch_cd_ref_contracts

diff_count=`diff -u ${FTP_DIR}"/"${YYYYMMDD:0:4}"/"${YYYYMMDD:4:2}"/"${YYYYMMDD:6:2}/contract \
            ${FTP_DIR}"/"${YYYY}"/"${MM}"/"${DD}/contract | wc -l`
diff_count2=`diff -u ${FTP_DIR}"/"${YYYYMMDD:0:4}"/"${YYYYMMDD:4:2}"/"${YYYYMMDD:6:2}/cd_contract \
            ${FTP_DIR}"/"${YYYY}"/"${MM}"/"${DD}/cd_contract | wc -l`

#[ ${diff_count} -ne 0 ] && echo "" | mailx -s "MD FILE UPDATED" -r sanjeev.kumar@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in
[[ ${diff_count} -ne 0 ]] && [[ $diff_count2 -ne 0 ]] && echo "" | mailx -s "Contract FILE UPDATED" -r $HOSTNAME raghunandan.sharma@tworoads-trading.co.in
#should get updated in next run
[[ ${diff_count} -eq 0 ]] || [[ ${diff_count2} -eq 0 ]] && { echo "FILE NOT UPDATED TRY AFTER SOMETIME"; exit;}

rm -rf /spare/local/tradeinfo/NSE_Files/RefData/nse_fo_$next_working_day"_contracts.txt"
rm -rf /spare/local/tradeinfo/NSE_Files/RefData/nse_cd_$next_working_day"_contracts.txt"

for i in FUTSTK OPTSTK FUTIDX OPTIDX FUTINT FUTIVX UNDINT ; do cat $FTP_DIR"/"${YYYY}"/"${MM}"/"${DD}"/contract" | awk -F"|" '{print $1,$2,$3,$4,$7,$8,$9,$54,$43,$44}' | grep "$i" >> /spare/local/tradeinfo/NSE_Files/RefData/nse_fo_$next_working_day"_contracts.txt" ; done
for i in FUTCUR FUTIRC FUTIRT INDEX OPTCUR UNDCUR UNDIRC UNDIRD UNDIRT; do cat $FTP_DIR"/"${YYYY}"/"${MM}"/"${DD}"/cd_contract" | awk -F"|" '{print $1,$2,$3,$4,$7,$8,$9,$54,$43,$44}' | grep "$i" >> /spare/local/tradeinfo/NSE_Files/RefData/nse_cd_$next_working_day"_contracts.txt" ; done

get_expiry_date;

echo "Expiry : $EXPIRY";
N_MM=$MM
N_YY=$YY
N_YYYY=$YYYY
if [ $YYYYMMDD -ge $EXPIRY ] ; then
  NEXT_MONTH=`date -d "$YYYYMMDD + 1 month - 10 day" +"%m%Y"`
  N_MM=${NEXT_MONTH:0:2}
  N_YY=${NEXT_MONTH:4:2}
  N_YYYY=${NEXT_MONTH:2:4}
#  fetch_mkt_lots ;
  echo "NextMonth: $N_MM , $N_YY"
fi
#Contract File Is Present, We can generate physical settlement names
ps_gr=`date -d  "${N_YYYY}-${N_MM}-01" "+%y%b" | awk '{print toupper($0)}'`
grep "$ps_grp" contract | grep "|P|" | awk -F"|" '{print $4}' | sort | uniq > /spare/local/tradeinfo/NSE_Files/SecuritiesUnderPhysicalSettlement/fo_securities_under_physical_settlement.csv

#Dated MktLots File
cp /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv" /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$next_working_day".csv";

