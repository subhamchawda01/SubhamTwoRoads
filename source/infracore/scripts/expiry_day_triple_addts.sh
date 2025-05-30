if [ $# -ne 2 ];
then
  echo "PROVIDE GROSS AND NET MARGIN VALUE IN THE ARGUEMENT";
fi
GROSS_MARGIN=$1;
NET_MARGIN=$2;
YYYYMMDD=`date +"%Y%m%d"`
HOSTNAME=`hostname`
ADDTS_FILE=/home/pengine/prod/live_configs/${HOSTNAME}_addts.cfg;

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
if [ $is_holiday = "1" ];then
  echo "NSE holiday..., exiting";
  exit
fi

EXPIRY=`grep IDXFUT /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.${YYYYMMDD} | grep BANKNIFTY | awk '{print $NF}' | sort | head -n1`
rm -rf /tmp/tmp_addts_file

if [ $YYYYMMDD -eq $EXPIRY ];
then
  cat ${ADDTS_FILE} | awk '{print $1,$2,$3,$4,3*$5,3*$6,3*$6,3*$7}' > /tmp/tmp_addts_file
  if [ -f /tmp/tmp_addts_file ];then
    /home/pengine/prod/live_scripts/ADDTRADINGSYMBOL.sh /tmp/tmp_addts_file
    echo " " | mailx -s "${HOSTNAME} : TRIPLE ADDTS IS DONE" -r sanjeev.kumar@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in
  fi
  if [ -z ${GROSS_MARGIN} ] || [ -z ${NET_MARGIN} ];
  then
    echo "" | mailx -s "${HOST_NAME} : TRIPPPLE ADDTS : FAILED UPDATING MARGIN" -r sanjeev.kumar@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in
    exit
  fi
# /home/pengine/prod/live_scripts/ors_control.pl NSE MSEQ2 SETNETMARGINCHECK ${NET_MARGIN}
# /home/pengine/prod/live_scripts/ors_control.pl NSE MSEQ2 SETGROSSMARGINCHECK ${GROSS_MARGIN}
fi
