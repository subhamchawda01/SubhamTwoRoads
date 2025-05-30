#!/bin/bash

date_=$1
#date_=20200323
MARGIN_DIR="/spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles/"
#MARGIN_DIR="/tmp/"
SECURITYMARGIN_FILE="${MARGIN_DIR}security_margin_${date_}.txt"
#addts_file="/home/dvcinfra/trash/unhedgedged_pos_addts_${date_}"
addts_file=$2
#cat /home/dvcinfra/important/nifty_banknifty_addts.txt >> ${addts_file}

if [ ! -s $addts_file ]; then
  echo "$addts_file is empty"
  exit
fi

for line in `awk '{print $4}' $addts_file | sed 's/"//g'`;
do
  prod_=`echo $line | cut -d_ -f1-2`
  margin_=`grep $prod_ $SECURITYMARGIN_FILE | head -1 | cut -d' ' -f2`
  sc_margin_="${line} ${margin_}"
  echo "grep -w $line $SECURITYMARGIN_FILE | wc -l"
  if [ `grep -w $line $SECURITYMARGIN_FILE | wc -l` == "0" ]; then
    echo "$line not present"
    ssh dvctrader@10.23.227.84 "echo ${sc_margin_} >> $SECURITYMARGIN_FILE"
  else
    echo "$line present"
  fi
done

 /home/pengine/prod/live_scripts/ors_control.pl NSE MSFO6 RELOADSECURITYMARGINFILE
sleep 3
/home/pengine/prod/live_scripts/ADDTRADINGSYMBOL.sh $addts_file
