#!/bin/bash

print_usage_and_exit () {
    echo "$0 YYYYMMDD" ;
    exit ;
}

#Main
if [ $# -ne 1 ] ; then
  echo "Called As : " $* ;
  print_usage_and_exit ;
fi

send_slack_exec=/home/pengine/prod/live_execs/send_slack_notification
YYYYMMDD=$1 ;
DD=${YYYYMMDD:6:2}
MM=${YYYYMMDD:4:2}
YY=${YYYYMMDD:2:2}
YYYY=${YYYYMMDD:0:4}
MSTR=$(echo $(date -d $YYYYMMDD +%b) | awk '{print toupper($1)}') ;

#chown dvctrader:infra $DD$MM"fo_0000".md
#mkdir -p /spare/local/tradeinfo/BSE_Files/BhavCopy/fo/$MM$YY/
#cp $DD$MM"fo_0000".md /spare/local/tradeinfo/BSE_Files/BhavCopy/fo/$MM$YY/

cd /spare/local/tradeinfo/BSE_Files/Intermediate_files/
#wget -O last_close_price_bkx_${YYYYMMDD}.txt "https://api.bseindia.com/BseIndiaAPI/api/ProduceCSVForDate/w?strIndex=SIBANK&dtFromDate=${DD}/${MM}/${YYYY}&dtToDate=${DD}/${MM}/${YYYY}"
#wget -O last_close_price_bsx_${YYYYMMDD}.txt "https://api.bseindia.com/BseIndiaAPI/api/ProduceCSVForDate/w?strIndex=SENSEX&dtFromDate=${DD}/${MM}/${YYYY}&dtToDate=${DD}/${MM}/${YYYY}"
token_=`/home/pengine/prod//live_execs/get_token_from_shortcode BSE_BSX_FUT0 $YYYYMMDD`
#token_=872189
close_price_=`grep -w $token_ /spare/local/files/BSEFTPFiles/$YYYY/$MM/$DD/EQD_DP$DD$MM$YY | cut -d ',' -f3 | awk '{print $1/100}'`
append_line_=`echo "$close_price_,$close_price_,$close_price_,$close_price_,0,0,0,0"`
#append_line_=`echo $append_line_ | awk '{print $1"0,0,0,0"}'`
echo $append_line_

cat /spare/local/files/BSEFTPFiles/${YYYY}/${MM}/${DD}/BSE_EQD_CONTRACT_${DD}${MM}${YYYY}.csv | grep -w SENSEX | awk -v line_=$append_line_ -F',' 'BEGIN {OFS = ",";} {print "NORMAL ",$3,"BSX",$5,$6/100,$7,"0",line_}' | sed 's/-/ /g' > $DD$MM"fo_0000".md

#append_line_=`cat last_close_price_bkx_${YYYYMMDD}.txt | grep -v Open | awk -F',' '{print $2","$3","$4","$5",0,0,0,0"}'`
token_=`/home/pengine/prod//live_execs/get_token_from_shortcode BSE_BKX_FUT0 $YYYYMMDD`
#token_=872446
close_price_=`grep -w $token_ /spare/local/files/BSEFTPFiles/$YYYY/$MM/$DD/EQD_DP$DD$MM$YY | cut -d ',' -f3 | awk '{print $1/100}'`
append_line_=`echo "$close_price_,$close_price_,$close_price_,$close_price_,0,0,0,0"`

cat /spare/local/files/BSEFTPFiles/${YYYY}/${MM}/${DD}/BSE_EQD_CONTRACT_${DD}${MM}${YYYY}.csv | grep -w BANKEX | awk -v line_=$append_line_ -F',' 'BEGIN {OFS = ",";} {print "NORMAL ",$3,"BKX",$5,$6/100,$7,"0",line_}' | sed 's/-/ /g'  >> $DD$MM"fo_0000".md

echo "cp ${DD}${MM}fo_0000.md /spare/local/tradeinfo/BSE_Files/BhavCopy/fo/$MM$YY/"
cp ${DD}${MM}fo_0000.md /spare/local/tradeinfo/BSE_Files/BhavCopy/fo/${MM}${YY}/

next_working_day=`/home/pengine/prod/live_execs/update_date $YYYYMMDD N W`

echo "Expiry : $EXPIRY";
EXPIRY=20290101 # NEED TO HANDLE IT FUTURE
N_MM=$MM
N_YY=$YY
N_YYYY=$YYYY
if [ $YYYYMMDD -ge $EXPIRY ] ; then
  NEXT_MONTH=`date -d "$YYYYMMDD + 1 month - 10 day" +"%m%Y"`
  N_MM=${NEXT_MONTH:0:2}
  N_YY=${NEXT_MONTH:4:2}
  N_YYYY=${NEXT_MONTH:2:4}
  echo "NextMonth: $N_MM , $N_YY"
fi


/home/pengine/prod/live_scripts/BSE_File_Generator.pl /spare/local/tradeinfo/BSE_Files/BhavCopy/fo/$MM$YY/$DD$MM"fo_0000".md /spare/local/tradeinfo/BSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv"  /spare/local/tradeinfo/BSE_Files/StrikeSchemeFiles/sos_scheme_$N_MM$N_YY".csv" /spare/local/tradeinfo/BSE_Files/ContractFiles/bse_contracts.${next_working_day} C /spare/local/tradeinfo/BSE_Files/datasource_exchsymbol.txt $next_working_day




