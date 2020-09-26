#!/bin/bash

DATE=`date +%Y%m%d`

if [ $# ! -eq 1 ] ; then
  echo "USAGE: <script> <short code file>"
  exit
fi

SHORT_CODE_FILE=$1;
ERROR_PROD_FILE="/tmp/error_prod"
ADDTS_ERROR_PROD_FILE="/tmp/addts_error_prod"
ADDTS_FILE="/tmp/addts_file"
ORS_DIR=`ps aux | grep cme | grep -v grep | awk '{print $15}'`
PROFILER=`echo $ORS_DIR | cut -d'/' -f6`
ORS_FILE="${ORS_DIR}/log.${DATE}"
echo "FILE: $ORS_FILE"
last_append_line=`grep -an "ORSLog in Append mode" $ORS_FILE | tail -1 | cut -d':' -f1`

#filter error products
echo "Symbol Error" >$ERROR_PROD_FILE
grep -an "Resetting" $ORS_FILE | awk -v line_no="$last_append_line" -F':' '{if($1>line_no) print $NF,"RESETTING" }' >>$ERROR_PROD_FILE
grep -an "MARGIN NOT PROVIDED" $ORS_FILE | grep "ERROR" | grep "ControlThread" | awk -v line_no="$last_append_line" -F':' '{if($1>line_no) print $NF,"MARGIN_NOT_PROVIDED" }' >>$ERROR_PROD_FILE
grep -an "UNDER BAN" $ORS_FILE | grep "ERROR" | grep "ControlThread" | awk -v line_no="$last_append_line" -F':' '{if($1>line_no) print $NF,"UNDER_BAN" }' >>$ERROR_PROD_FILE

#filter addts products
awk '(NR>1 && NR==FNR) { prod[$1]=$2; next } { if ($1 in prod) { print $1,prod[$1],$2}}' $ERROR_PROD_FILE  $SHORT_CODE_FILE > $ADDTS_ERROR_PROD_FILE

#addts of products
while IFS= read -r prod_code error position
do
  margin_file="/home/pengine/prod/live_configs/common_initial_margin_file.txt"
  if [ $error == "RESETTING" ]; then
    if [ `grep -w $prod_code $margin_file | wc -l` == "1" ]; then
      max_pos=`expr 3 \* $(grep -w $prod_code $margin_file | awk 'function max(a,b) { return a>b ? a:b } {print max(max($2,$3),$4)}')`
      replace_margin=`grep -w $prod_code $margin_file`
      replace_margin_with="$prod_code $max_pos $max_pos $max_pos"
      sed -i "s/$replace_margin/$replace_margin_with/g" $margin_file
    else
      append_line=`grep -n "NSE_" $margin_file | tail -1 | cut -d':' -f1`
      sed -i "$append_line a $prod_code 300 300 300"
    fi
    /home/pengine/prod/live_scripts/ors_control.pl NSE $PROFILER RELOADMARGINFILE

  elif [ $error == "MARGIN_NOT_PROVIDED" ]; then
    securitymargin_file="/spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles/security_margin_${DATE}.txt"
    prod=`echo $prod_code $securitymargin_file | cut -d_ -f1-2`
    margin=`grep $prod $securitymargin_file | head -1 | cut -d' ' -f2`
    sc_margin="$prod_code $margin"

    if [ `grep -w $prod_code $securitymargin_file | wc -l` == "0" ]; then
      echo ${sc_margin} >> $securitymargin_file
    fi
    /home/pengine/prod/live_scripts/ors_control.pl NSE $PROFILER RELOADSECURITYMARGINFILE

  elif [ $error == "UNDER_BAN" ] ; then
    under_ban_file="/spare/local/tradeinfo/NSE_Files/SecuritiesUnderBan/fo_secban_${DATE}.csv"
    prod_name=`echo $prod_code | cut -d_ -f2`
    sed -i "/$prod_name/d" $under_ban_file
    /home/pengine/prod/live_scripts/ors_control.pl NSE $PROFILER RELOADMARGINFILE
  fi
done < $ADDTS_ERROR_PROD_FILE

awk -v prof="$PROFILER" 'function abs(pos) {return (pos < 0 ? -pos : pos)} {print "NSE",prof,"ADDTRADINGSYMBOL",$1,abs($3),abs($3),abs($3),abs($3)}' $ADDTS_ERROR_PROD_FILE > $addts_file

/home/pengine/prod/live_scirpts/ADDTRADINGSYMBOL.sh $addts_file
