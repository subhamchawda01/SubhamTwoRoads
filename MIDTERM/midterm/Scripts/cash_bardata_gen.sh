#!/bin/bash
if [ $# -ne 1 ];then
  echo "Arguement date(YYYYMMDD) required, Exiting ...";
  exit
fi

GetPreviousWorkingDay() {
  prev_workingday=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P W`
  is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $prev_workingday T`
  while [ $is_holiday_ = "1" ];
  do
    prev_workingday=`/home/pengine/prod/live_execs/update_date $prev_workingday P W`
    is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $prev_workingday T`
  done
  echo "Previous Workingday $prev_workingday"
}

YYYYMMDD=$1;
is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
GetPreviousWorkingDay
if [ $is_holiday_ = "1" ];
then
	echo "NSE Holiday, Exiting..."
	exit;
fi
YYYY=${YYYYMMDD:0:4};
MM=${YYYYMMDD:4:2};
DD=${YYYYMMDD:6:2};
TMP_WORK_DIR="/home/dvcinfra/trash/cash_temp";
PRODUCTS_DATA_FILE="${TMP_WORK_DIR}"/Filenames
GENERATED_DATA_DIR="${TMP_WORK_DIR}"/Generated_Data/
SORTED_DATA_DIR="${TMP_WORK_DIR}"/Sorted_Data/
BAR_DATA_DESTINATION_DIR="/NAS1/data/NSEBarData/CASH_BarData/"
HFT_DATA_DIR="/NAS1/data/NSELoggedData/NSE/${YYYY}/${MM}/${DD}/"
BAR_DATA_GENERATOR_EXEC="/home/pengine/prod/live_execs/historical_data_generator_cash_new"
#BAR_DATA_GENERATOR_EXEC="/home/dvcinfra/raghu/historical_data_generator_cash_new"
#create temp work dir
mkdir -p $GENERATED_DATA_DIR $SORTED_DATA_DIR 

#Clean Up the temp work dir
rm -rf $PRODUCTS_FILE $GENERATED_DATA_DIR/* $SORTED_DATA_DIR/*

send_alert_and_exit() {
  msg=$1;
  echo "" | mailx -s "Date $YYYYMMDD ${msg}" -r "${HOSTNAME}-${USER}<sanjeev.kumar@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in
  exit;
}

#check all requirements are met
[ -d $HFT_DATA_DIR ] || send_alert_and_exit "CASH DATA GEN :  HFT DATA DOESNT EXISTS";

echo "GETTING PROD FILE $PRODUCTS_DATA_FILE"
>$PRODUCTS_DATA_FILE
for file in `ls $HFT_DATA_DIR | egrep -v "_CE_|_PE_" |grep -v "_FUT_" `;do
      product=`echo $file|cut -d'_' -f2`
      if ! grep -q $product "/spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_${prev_workingday}.csv"; then
          echo "${HFT_DATA_DIR}${file}" >>$PRODUCTS_DATA_FILE;
      fi
done

[ -f $PRODUCTS_DATA_FILE  -a -s $PRODUCTS_DATA_FILE ] || send_alert_and_exit "Cash DATA GEN : FAILED TO GENERATE PRODUCTS FILE";
files_count=`wc -l ${PRODUCTS_DATA_FILE} | awk '{print $1}'`
[ $files_count -lt 500 ] && send_alert_and_exit "CASH BAR DATA GENERATION  FILE COUNT IS LESS THAN 500";

#cash start at 3:37
start_=$(date -d "$YYYYMMDD 0337" +%s);
echo "Removing Corrupt Data from the older files with start $start_"
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
# <EXEC> <DATE> <PRODUCTS DATA FILE> <OUTPUT DIR>
echo "$BAR_DATA_GENERATOR_EXEC $YYYYMMDD $PRODUCTS_DATA_FILE $GENERATED_DATA_DIR"
$BAR_DATA_GENERATOR_EXEC $YYYYMMDD $PRODUCTS_DATA_FILE $GENERATED_DATA_DIR
EXIT_STATUS=$?;
[ $EXIT_STATUS -eq 0 ] || send_alert_and_exit "CASH DATA GEN :  EXEC HAS FAILED TO GENERATE";

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

echo -e "BARDATACOUNT : $bardata_count\nNO_OF_PRODUCT : $no_of_prod_" | mailx -s " CASH DATA GEN successfully $YYYYMMDD" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in 

