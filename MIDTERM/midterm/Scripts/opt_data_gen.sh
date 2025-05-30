#!/bin/bash

: '
	if it is running from the cront then it willl generate for previous working day,
	to generate for specific date , pass date in the arguement to generate for specific
	day
'

GetPreviousWorkingDay() {
  YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P W`
  is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  while [ $is_holiday_ = "1" ];
  do
    YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P W`
    is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  done
}


YYYYMMDD=`date +"%Y%m%d"`;
if [ $# -eq 1 ];then
	YYYYMMDD=$1;
else
	GetPreviousWorkingDay;
fi

is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
if [ $is_holiday = "1" ];
then
	echo "NSE Holiday, Exiting..."
	exit;
fi
#initialisation
USER=`whoami`
YYYY=${YYYYMMDD:0:4};
MM=${YYYYMMDD:4:2};
DD=${YYYYMMDD:6:2};
TMP_WORK_DIR="/home/${USER}/trash/temp_options";
PRODUCTS_DATA_FILE="${TMP_WORK_DIR}"/Filenames
GENERATED_DATA_DIR="${TMP_WORK_DIR}"/Generated_Data/
SORTED_DATA_DIR="${TMP_WORK_DIR}"/Sorted_Data/
BAR_DATA_DESTINATION_DIR="/NAS1/data/NSEBarData/OPT_BarData/"
HFT_DATA_DIR="/NAS1/data/NSELoggedData/NSE/${YYYY}/${MM}/${DD}/"
EXPIRY_FILE="${TMP_WORK_DIR}"/monthly_expiries_options.txt
CONTRACT_FILE="/spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts."${YYYYMMDD}
BAR_DATA_GENERATOR_EXEC="/home/${USER}/cvquant_install/midterm/bin/nse_historical_data_generator_options"


#create temp work dir
mkdir -p $GENERATED_DATA_DIR $SORTED_DATA_DIR 

#Clean Up the temp work dir
rm -rf $PRODUCTS_FILE $GENERATED_DATA_DIR/* $SORTED_DATA_DIR/*

send_alert_and_exit() {
  msg=$1;
  echo "" | mailx -s "${msg}" -r sanjeev.kumar@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in
  exit;
}
#get expiries from contract file
cat $CONTRACT_FILE | grep STKOPT | awk '{print $6}' | sort | uniq > ${EXPIRY_FILE};

#check all requirements are met
[ -d $HFT_DATA_DIR ] || send_alert_and_exit "OPT DATA GEN :  HFT DATA DOESNT EXISTS";


[ -f $EXPIRY_FILE ] || send_alert_and_exit "OPT DATA GEN :  FAILED TO GENERATE EXPIRY FILE"; 

[ -f $BAR_DATA_GENERATOR_EXEC ] || send_alert_and_exit "OPT DATA GEN :  EXEC NOT FOUND";


#get the file names
expiry_list_string=`cat ${EXPIRY_FILE} | xargs | tr ' ' '|'`

find $HFT_DATA_DIR -type f | egrep "_CE_|_PE_" | egrep "${expiry_list_string}" > $PRODUCTS_DATA_FILE

[ -f $PRODUCTS_DATA_FILE  -a -s $PRODUCTS_DATA_FILE ] || send_alert_and_exit "OPT DATA GEN : FAILED TO GENERATE PRODUCTS FILE";

files_count=`wc -l ${PRODUCTS_DATA_FILE} | awk '{print $1}'`
[ $files_count -lt 30000 ] && send_alert_and_exit "OPT DATA GEN :  FILE COUNT IS LESS THAN 30000";

#generate bar data
# <EXEC> <DATE> <PRODUCTS DATA FILE> <EXPIRY FILE> <OUTPUT DIR>
$BAR_DATA_GENERATOR_EXEC $YYYYMMDD $PRODUCTS_DATA_FILE $EXPIRY_FILE $GENERATED_DATA_DIR

EXIT_STATUS=$?;
[ $EXIT_STATUS -eq 0 ] || send_alert_and_exit "OPT DATA GEN :  EXEC HAS FAILED TO GENERATE";
#sort generated data on timestamp
cd $GENERATED_DATA_DIR

for FILE in `ls`;
do
  sort -n -k 1 $FILE >> ${SORTED_DATA_DIR}/$FILE
done
cd ${SORTED_DATA_DIR}

for file in `ls`;
do 
  cat $file >> ${BAR_DATA_DESTINATION_DIR}/$file
done

rm -rf $TMP_WORK_DIR
