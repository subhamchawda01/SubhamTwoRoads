#!/bin/bash


DATE=`date +%Y%m%d`
UNDER_PHY_ADDTS_ERROR_FILE="/tmp/mail_phy_addts_file"
UNDER_BAN_ADDTS_ERROR_FILE="/tmp/mail_under_ban_addts_file"
SEC_MARGIN_ADDTS_ERROR_FILE="/tmp/mail_sec_margin_addts_file"
RESETTING_ADDTS_FILE="/tmp/resetting_addts_file"
UNDER_UNSOLI_ADDTS_ERROR_FILE="/tmp/mail_under_unsoli_addts_file"
ADDTS_ERROR_FILE="/tmp/verify_addt_mail_file_`hostname`_${DATE}"

>$ADDTS_ERROR_FILE
>$UNDER_PHY_ADDTS_ERROR_FILE
>$UNDER_BAN_ADDTS_ERROR_FILE
>$SEC_MARGIN_ADDTS_ERROR_FILE
>$UNDER_UNSOLI_ADDTS_ERROR_FILE
>$RESETTING_ADDTS_FILE

ORS_DIR=`ps aux | grep cme | grep -v grep | awk '{print $15}'`
ORS_FILE="${ORS_DIR}/log.${DATE}"
echo "FILE: $ORS_FILE"
last_append_line=`grep -an "ORSLog in Append mode" $ORS_FILE | tail -1 | cut -d':' -f1`

for prod in `grep -an "ADDTRADINGSYMBOL" $ORS_FILE | awk -v line_app_no="$last_append_line" -F':' '{if($1>line_app_no) print $2 }' | awk '{print $4}' | sort | uniq | grep NSE `; do

  addts_line_=`grep -an "ADDTRADINGSYMBOL" $ORS_FILE | grep -w $prod | tail -1 | cut -d':' -f1`

  echo "PROD: $prod : $addts_line_"
  grep -an Resetting $ORS_FILE | awk -v line_no="$addts_line_" -F':' '{if($1>line_no) print $0 }' | grep -w $prod >/tmp/resetting_file
  reset_pos_error_=`grep "Requested max position value" /tmp/resetting_file | grep $prod | tail -1`
  reset_maxorder_error_=`grep "Requested max order size value" /tmp/resetting_file | grep $prod | tail -1`
  reset_order_error_=`grep "Requested order size value" /tmp/resetting_file | grep $prod | tail -1`
  
  if [[ ! -z $reset_pos_error_ ]]; then
    echo "$reset_pos_error_" | awk '{prod=NF-5;req=NF-13; print "Requested max pos: "$req" "$prod" Reset_value: "$NF}' >>$RESETTING_ADDTS_FILE
  fi
  if [[ ! -z $reset_maxorder_error_ ]]; then
    echo "$reset_maxorder_error_" | awk '{prod=NF-4;req=NF-12; print "Requested max order: "$req" "$prod" Reset_value: "$NF}' >>$RESETTING_ADDTS_FILE
  fi
  if [[ ! -z $reset_order_error_ ]]; then
    echo "$reset_order_error_" | awk '{prod=NF-6;req=NF-14; print "Requested order: "$req" "$prod" Reset_value: "$NF}' >>$RESETTING_ADDTS_FILE
  fi

  error_=`awk '{if(NR>=1) print $0}' $ORS_FILE | grep -n ERROR | grep ControlThread | grep -w $prod | awk -v line_no="$addts_line_" -F':' '{if($1>line_no) print $0 }' | tail -1`

  if [[ ! -z $error_ ]]; then
    echo "prod: $prod"
echo "margin:: $error_"
#echo "ERROR:: $error_" 
    if [ `echo $error_ | grep "MARGIN NOT PROVIDED" | wc -l` == "1" ]; then
      echo "$error_" | awk '{print $17}' >>$SEC_MARGIN_ADDTS_ERROR_FILE
    elif [ `echo $error_ | grep "UNDER BAN" | wc -l` == "1" ]; then
      echo "$error_" | awk '{print $7}' >>$UNDER_BAN_ADDTS_ERROR_FILE
    elif [ `echo $error_ | grep "UNDER PHYSICAL SETTLEMENT" | wc -l` == "1" ]; then
      echo "$error_" | awk '{print $7}' >>$UNDER_PHY_ADDTS_ERROR_FILE
    elif [ `echo $error_ | grep "UNDER UNSOLICITED SMS CIRCULATION" | wc -l` == "1" ]; then
      echo "$error_" | awk '{print $7}' >>$UNDER_UNSOLI_ADDTS_ERROR_FILE
    fi
  fi
done

if [ ! -s $UNDER_PHY_ADDTS_ERROR_FILE ] && [ ! -s $UNDER_BAN_ADDTS_ERROR_FILE ] && [ ! -s $SEC_MARGIN_ADDTS_ERROR_FILE ] && [ ! -s $UNDER_UNSOLI_ADDTS_ERROR_FILE ] && [ ! -s $RESETTING_ADDTS_FILE ]; then
  echo "ALL FILES ARE EMPTY"
  echo "ADDTS VERIFICATION DONE" >>$ADDTS_ERROR_FILE
  #echo "" | mail -s "ADDTS DONE SUCCESSFULLY on ${HOSTNAME}" hardik.dhakate@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in nseall@tworoads.co.in
  exit;
fi

if [ -s $SEC_MARGIN_ADDTS_ERROR_FILE ]; then
  echo "MARGIN FILE"
  if [ -s $SEC_MARGIN_ADDTS_ERROR_FILE ]; then
    echo -e "\n====== MARGIN NOT PROVIDED ======\n" >>$ADDTS_ERROR_FILE
    cat $SEC_MARGIN_ADDTS_ERROR_FILE
    /home/pengine/prod/live_scripts/orsRejectHandling.sh margin $SEC_MARGIN_ADDTS_ERROR_FILE
#     /home/dvcinfra/important/subham/orsRejectHandling.sh margin $SEC_MARGIN_ADDTS_ERROR_FILE
    >$SEC_MARGIN_ADDTS_ERROR_FILE
    cat $SEC_MARGIN_ADDTS_ERROR_FILE >>$ADDTS_ERROR_FILE
    echo -e "\n" >>$ADDTS_ERROR_FILE
  fi
fi

if [ -s $UNDER_BAN_ADDTS_ERROR_FILE ]; then
  echo "UNDER BAN FILE"
  echo -e "\n====== UNDER BAN ======\n" >>$ADDTS_ERROR_FILE
  cat $UNDER_BAN_ADDTS_ERROR_FILE
  cat $UNDER_BAN_ADDTS_ERROR_FILE >>$ADDTS_ERROR_FILE
  echo -e "\n" >>$ADDTS_ERROR_FILE
fi

if [ -s $RESETTING_ADDTS_FILE ]; then
  echo "RESETTING FILE"
  if [ -s $RESETTING_ADDTS_FILE ]; then
    echo -e "\n======RESETTING POSITIONS======\n" >>$ADDTS_ERROR_FILE
    cat $RESETTING_ADDTS_FILE
    awk '{print $(NF-2)}' $RESETTING_ADDTS_FILE | sort | uniq | sed 's/\.//' > /tmp/verify_resetting_error
    /home/pengine/prod/live_scripts/orsRejectHandling.sh resetting /tmp/verify_resetting_error
#    /home/dvcinfra/important/subham/orsRejectHandling.sh resetting $RESETTING_ADDTS_FILE
    >$RESETTING_ADDTS_FILE
    cat $RESETTING_ADDTS_FILE >>$ADDTS_ERROR_FILE
    echo -e "\n" >>$ADDTS_ERROR_FILE
  fi
fi

if [ -s $UNDER_UNSOLI_ADDTS_ERROR_FILE ]; then
  echo "UNDER BAN UNSOLICITED FILE"
  echo -e "\n====== UNDER UNSOLICITED SMS CIRCULATION ======\n" >>$ADDTS_ERROR_FILE
  cat $UNDER_UNSOLI_ADDTS_ERROR_FILE
  cat $UNDER_UNSOLI_ADDTS_ERROR_FILE >>$ADDTS_ERROR_FILE
  echo -e "\n" >>$ADDTS_ERROR_FILE
fi  

if [ -s $UNDER_PHY_ADDTS_ERROR_FILE ]; then
  echo "UNDER PHYSICAL FILE"
  echo -e "\n====== UNDER PHYSICAL SETTLEMENT ======\n" >>$ADDTS_ERROR_FILE
  cat $UNDER_PHY_ADDTS_ERROR_FILE
  cat $UNDER_PHY_ADDTS_ERROR_FILE >>$ADDTS_ERROR_FILE
  echo -e "\n" >>$ADDTS_ERROR_FILE
fi

echo "ADDTS VERIFICATION DONE" >>$ADDTS_ERROR_FILE
#cat $ADDTS_ERROR_FILE | mail -s "ADDTS ERROR on ${HOSTNAME}" hardik.dhakate@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in nseall@tworoads.co.in

