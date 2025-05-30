#!/bin/bash
if [ $# -ne 1 ];then
  echo "Arguement date(YYYYMMDD) required, Exiting ...";
  exit
fi
#initialisation
USER=`whoami`
YYYYMMDD=$1;
is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`

if [ $is_holiday_ = "1" ];
then
	echo "NSE Holiday, Exiting..."
	exit;
fi
YYYY=${YYYYMMDD:0:4};
MM=${YYYYMMDD:4:2};
DD=${YYYYMMDD:6:2};
TMP_WORK_DIR="/home/${USER}/trash/temp";
PRODUCTS_DATA_FILE="${TMP_WORK_DIR}"/Filenames
GENERATED_DATA_DIR="${TMP_WORK_DIR}"/Generated_Data/
SORTED_DATA_DIR="${TMP_WORK_DIR}"/Sorted_Data/
BAR_DATA_DESTINATION_DIR="/NAS1/data/NSEBarData/FUT_BarData/"
HFT_DATA_DIR="/NAS1/data/NSELoggedData/NSE/${YYYY}/${MM}/${DD}/"
EXPIRY_FILE="${TMP_WORK_DIR}"/monthly_expiries.txt
CONTRACT_FILE="/spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts."${YYYYMMDD}
BAR_DATA_GENERATOR_EXEC="/home/pengine/prod/live_execs/nse_historical_data_generator_fut"
MAIL_SENDER=("sanjeev.kumar@tworoads-trading.coin")
MAIL_RECEIVER=("sanjeev.kumar@tworoads-trading.co.in");
#create temp work dir
mkdir -p $GENERATED_DATA_DIR $SORTED_DATA_DIR 

#Clean Up the temp work dir
rm -rf $PRODUCTS_FILE $GENERATED_DATA_DIR/* $SORTED_DATA_DIR/*

send_alert_and_exit() {
  msg=$1;
  echo "" | mailx -s "${msg}" -r "${HOSTNAME}-${USER}<sanjeev.kumar@tworoads-trading.co.in>" sanjeev.kumar@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in
  exit;
}

#get expiries from contract file
cat $CONTRACT_FILE | grep IDXFUT | grep BANKNIFTY | awk '{print $NF}' > ${EXPIRY_FILE};
#check all requirements are met
[ -d $HFT_DATA_DIR ] || send_alert_and_exit "FUT DATA GEN :  HFT DATA DOESNT EXISTS";


[ -f $EXPIRY_FILE ] || send_alert_and_exit "FUT DATA GEN :  FAILED TO GENERATE EXPIRY FILE"; 

[ -f $BAR_DATA_GENERATOR_EXEC ] || send_alert_and_exit "FUT DATA GEN :  EXEC NOT FOUND";

#get the file names
find $HFT_DATA_DIR -type f | grep _FUT_ > $PRODUCTS_DATA_FILE

[ -f $PRODUCTS_DATA_FILE  -a -s $PRODUCTS_DATA_FILE ] || send_alert_and_exit "FUT DATA GEN : FAILED TO GENERATE PRODUCTS FILE";

files_count=`wc -l ${PRODUCTS_DATA_FILE} | awk '{print $1}'`

[ $files_count -lt 400 ] && send_alert_and_exit "BAR DATA GENERATION  FILE COUNT IS LESS THAN 500";


start_=$(date -d "$YYYYMMDD 0345" +%s);
for prod in `ls $BAR_DATA_DESTINATION_DIR`; 
do 
  lineNum=$(awk -v start_="$start_" '$1>=start_{print NR;exit}' "${BAR_DATA_DESTINATION_DIR}${prod}"); 
#echo "$prod $lineNum" ; 
  if [ -z "$lineNum" ]
  then 
    echo "no corrupt data for $prod"
  else
    echo "corrupted data for $prod"
    sed -i "$lineNum,\$d" "${BAR_DATA_DESTINATION_DIR}${prod}"
  fi
done


#generate bar data
# <EXEC> <DATE> <PRODUCTS DATA FILE> <EXPIRY FILE> <OUTPUT DIR>
$BAR_DATA_GENERATOR_EXEC $YYYYMMDD $PRODUCTS_DATA_FILE $EXPIRY_FILE $GENERATED_DATA_DIR
EXIT_STATUS=$?;
[ $EXIT_STATUS -eq 0 ] || send_alert_and_exit "FUT DATA GEN :  EXEC HAS FAILED TO GENERATE";

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


today_start_time=`date -d"$YYYYMMDD" +"%s"`;
today_end_time=$((today_start_time+35940));

bardata_count=0;

for prod in `ls $BAR_DATA_DESTINATION_DIR`;
do
  count=`grep $today_end_time ${BAR_DATA_DESTINATION_DIR}$prod | head -1 | wc -l`
  [ $count == "1" ] && ((bardata_count++)) ;
done

no_of_prod_=`ls -lrt ${BAR_DATA_DESTINATION_DIR} | grep -v total | wc -l` ;

echo -e "BARDATACOUNT : $bardata_count\nNO_OF_PRODUCT : $no_of_prod_" | mailx -s " FUT DATA GEN successful " -r "${HOSTNAME}-${USER}<sanjeev.kumar@tworoads-trading.co.in>" sanjeev.kumar@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in

