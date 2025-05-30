#!/bin/bash
if [ $# -ne 1 ];then
  echo "Date(yyyymmdd) arguement required ";
  exit
fi

YYYYMMDD=$1

NEXT_DAY=`/home/pengine/prod/live_execs/update_date $YYYYMMDD N W`
LOG_FILE='/home/dvcinfra/trash/bar_data_adjust.log'
START_DATE=20050101;
FUT_BAR_DATA_DIR="/NAS1/data/NSEBarData/FUT_BarData/"
ADJUSTED_BAR_DATA_DEST_DIR="/NAS1/data/NSEBarData/FUT_BarData_Adjusted/"
ADJUSTED_BAR_DATA_DEST_BKP_DIR="/NAS1/data/NSEBarData/FUT_BarData_Adjusted_bkp/"
CORP_ACTION_FILE="/NAS1/data/NSEMidTerm/MachineReadableCorpAdjustmentFiles/Complete_Index_No_Header.csv"

rm -rf $LOG_FILE
#generate adjusted data
mv $ADJUSTED_BAR_DATA_DEST_DIR/*  $ADJUSTED_BAR_DATA_DEST_BKP_DIR

cd $FUT_BAR_DATA_DIR
for ticker in `ls -p | grep -v "/"`;
do
  /home/pengine/prod/live_execs/backadjustment_exec --start_date $START_DATE --end_date $NEXT_DAY --index_file $CORP_ACTION_FILE --output_file $ADJUSTED_BAR_DATA_DEST_DIR"${ticker}"  --ticker $ticker --header 0
  [ $? -ne 0 ] && echo $ticker >> ${LOG_FILE}
done

[ -f $LOG_FILE ] && cat $LOG_FILE | mailx -s "FAILED BAR DATA AJUSTMENT" -r "${HOSTNAME}-${USER}<sanjeev.kumar@tworoads-trading.co.in>" sanjeev.kumar@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in 

