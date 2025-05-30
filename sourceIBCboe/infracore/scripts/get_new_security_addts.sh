#!/bin/bash

declare -A server_to_profile_
server_to_profile_=( ["sdv-ind-srv14"]="MSFO7" \
                     ["sdv-ind-srv15"]="MSFO4" \
                     ["sdv-ind-srv16"]="MSEQ2" \
                     ["sdv-ind-srv17"]="MSEQ3" \
                     ["sdv-ind-srv18"]="MSEQ4" \
                     ["sdv-ind-srv23"]="MSEQ6" \
                     ["sdv-ind-srv19"]="MSFO5" \
                     ["sdv-ind-srv20"]="MSFO6" )

declare -A server_to_ip_map
server_to_ip_map=( ["sdv-ind-srv14"]="10.23.227.64" \
                   ["sdv-ind-srv15"]="10.23.227.65" \
                   ["sdv-ind-srv16"]="10.23.227.81" \
                   ["sdv-ind-srv17"]="10.23.227.82" \
                   ["sdv-ind-srv23"]="10.23.227.72" \
                   ["sdv-ind-srv18"]="10.23.227.83" \
                   ["sdv-ind-srv19"]="10.23.227.69" \
                   ["sdv-ind-srv20"]="10.23.227.84" )

profile_=${server_to_profile_[`hostname`]}
server_ip=${server_to_ip_map[`hostname`]}
echo "$profile_ :: $server_ip"

DATE=`date +%Y%m%d`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $DATE T`
if [ $is_holiday = "1" ];then
  echo "NSE holiday..., exiting";
  exit
fi

PREV_DATE=`/home/pengine/prod/live_execs/update_date $DATE P A`;
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $PREV_DATE T`
while [ $is_holiday = "1" ];
do
  PREV_DATE=`/home/pengine/prod/live_execs/update_date $PREV_DATE P A`
  is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $PREV_DATE T`
done

NSE_FILE_DIR="/spare/local/tradeinfo/NSE_Files/"
SECURITY_MARGIN_FILE="${NSE_FILE_DIR}SecuritiesMarginFiles/security_margin_${DATE}.txt"
SECURITY_AVAILABLE_LIST="${NSE_FILE_DIR}SecuritiesAvailableForTrading/sec_list.csv_${DATE}"
OLD_SECURITY_AVAILABLE_LIST="${NSE_FILE_DIR}SecuritiesAvailableForTrading/sec_list.csv_${PREV_DATE}"
MAIL_FILE="/tmp/sec_margin_mail_${DATE}.txt"
ADDTS_FILE="/tmp/new_sec_avail_addts.txt"
>${ADDTS_FILE}
rm ${MAIL_FILE}
#ls -lrt $MAIL_FILE
diff -u ${OLD_SECURITY_AVAILABLE_LIST} ${SECURITY_AVAILABLE_LIST} | grep -v "\-\-\-" | grep -v "+++" | grep -v @ | grep "^+" | awk '{print substr($1,2);}' >/tmp/sec_list_diff.txt

for prod in `cat /tmp/sec_list_diff.txt`; do
  nse_prod="NSE_${prod}"
  if [ `grep -w $nse_prod $SECURITY_MARGIN_FILE | wc -l` == "0" ]; then
    margin_=`grep -w $nse_prod "${NSE_FILE_DIR}SecuritiesMarginFiles/security_margin_"* | tail -1 | cut -d' ' -f2`
    if [ "$margin_" != "" ]; then
      echo "MARGIN_ADDED $prod $margin_" >>${MAIL_FILE}
    else
      echo "MARGIN_ADDED $prod 20" >>${MAIL_FILE}
      margin_="20"
    fi
      sc_margin_="${nse_prod} ${margin_}"
      ssh dvctrader@${server_ip} "echo ${sc_margin_} >> $SECURITY_MARGIN_FILE"
      echo "NSE ${profile_} ADDTRADINGSYMBOL $nse_prod 5000 5000 5000 10000" >> ${ADDTS_FILE} 
  else
    echo "MARGIN_PRESENT $prod" >> ${MAIL_FILE}
  fi
done

/home/pengine/prod/live_scripts/ors_control.pl NSE ${profile_} RELOADSECURITYMARGINFILE
sleep 3 
/home/pengine/prod/live_scripts/ADDTRADINGSYMBOL.sh ${ADDTS_FILE}
sleep 3 
ssh dvctrader@${server_ip} "if [ -f /tmp/addts_file ]; then /home/pengine/prod/live_scripts/ADDTRADINGSYMBOL.sh /tmp/addts_file; fi"

sleep 3 
ssh dvctrader@${server_ip} "if [ -f /tmp/asm_addts.cfg ]; then /home/pengine/prod/live_scripts/ADDTRADINGSYMBOL.sh /tmp/asm_addts.cfg; fi"
