#!/bin/bash

print_usage_and_exit () {
    echo "$0 YYYYMMDD" ;  
    exit ; 
} 

FTP_HOST='https://member.bseindia.com/' # CUrrently not using this.. should find a way to use through this also..

#DON"T KNOW ABOUT LOT SIZE FILE
fetch_mkt_lots () {

 # /spare/local/tradeinfo/BSE_Files/Lotsizes 
  echo "LOTSIZE"
}

check_bhavcopy_expiry () {
least_expiry=`cat $1 | cut -d',' -f3 | cut -d' ' -f3 | sort | head -1`
    if [ $least_expiry -lt $YYYY ]; then
      printf "Older expirires present in bhavcopy file" | /bin/mail -s "BSEDailyFilesGenerationFailure - $1" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in;
#      exit;
    fi
}

# TODO FILE CREATE REF FILES AND 000.md files

fetch_cm_scrip () {
  rm scrip.zip
  while true
  do
    wget -t 2 --timeout=20 --referer https://www.bseindia.com --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www.bseindia.com/downloads/Help/file/scrip.zip

    if [[ $? -ne 0 ]]
    then
      break
    else
      sleep 15m
    fi
done

#TODO check files based on file update date FO<CM<CD
  unzip scrip.zip
  rm scrip.zip

#  get security.gz 
}

fetch_fo_scrip () {
    rm CO.zip
    wget -t 2 --timeout=20 --referer https://www.bseindia.com --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0"  https://www.bseindia.com/downloads/Help/file/CO.zip
    unzip CO.zip
    rm CO.zip

#  get EQD_DP250621
#  get EQD_CO250621.csv
#  get EQD_SPD_CO250621.csv
}


fetch_cd_scrip () {
  rm Master.zip
  wget -t 2 --timeout=20 --referer https://www.bseindia.com --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www.bseindia.com/downloads1/Master.zip
  unzip Master.zip
  rm Master.zip 
#  get BFX_SPD_CO250621.csv
#  get BFX_DP250621
#  get BFX_CO250621.csv

} 

fetch_cm_bhavcopy () {
  wget -t 2 --timeout=20 --referer https://www.bseindia.com --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www.bseindia.com/download/BhavCopy/Equity/EQ${DD}${MM}${YY}_CSV.ZIP
    unzip -o EQ${DD}${MM}${YY}_CSV.ZIP;
    rm EQ${DD}${MM}${YY}_CSV.ZIP
}

fetch_fo_bhavcopy () {
  wget -t 2 --timeout=20 --referer https://www.bseindia.com --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www.bseindia.com/download/Bhavcopy/Derivative/bhavcopy${DD}-${MM}-${YY}.zip
    unzip -o bhavcopy${DD}-${MM}-${YY}.zip ;
    rm bhavcopy${DD}-${MM}-${YY}.zip;
}


fetch_cd_bhavcopy () {
  wget -t 2 --timeout=20 --referer https://www.bseindia.com --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www.bseindia.com/bsedata/CIML_bhavcopy/CurrencyBhavCopy_${YYYY}${MM}${DD}.zip
  unzip -o CurrencyBhavCopy_${YYYY}${MM}${DD}.zip ;
  rm CurrencyBhavCopy_${YYYY}${MM}${DD}.zip;

}

verify_FTP_files () {
    GIVEN_FILE=${FTP_DIR}${1};
        [ -f $GIVEN_FILE -a -s $GIVEN_FILE -a -r $GIVEN_FILE ] || printf "$GIVEN_FILE FTP FILE not present for today" | /bin/mail -s "BSEDailyFilesGenerationFailure - $GIVEN_FILE" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" "raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in"
#                [ -f $GIVEN_FILE -a -s $GIVEN_FILE -a -r $GIVEN_FILE ] || exit ;
}

verify_given_files () {
  GIVEN_FILE=$1;
    [ -f $GIVEN_FILE -a -s $GIVEN_FILE -a -r $GIVEN_FILE ] || printf "File not present for today $GIVEN_FILE" | /bin/mail -s "BSEDailyFilesGenerationFailure - $GIVEN_FILE" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" "raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in"
#    [ -f $GIVEN_FILE -a -s $GIVEN_FILE -a -r $GIVEN_FILE ] || exit ;
}

get_expiry_date () {
  ONE=01;
  EXPIRY=$YYYYMMDD;
  for i in {1..7}; do dateStr=`date -d "$YYYY$MM$ONE + 1 month - $i day" +"%w %Y%m%d"`; if [ ${dateStr:0:1} -eq 4 ] ; then EXPIRY=${dateStr:2}; fi ; done
}

#Main 
if [ $# -ne 1 ] ; then 
  echo "Called As : " $* ; 
  print_usage_and_exit ;
fi 
echo "---------------------------------------------Starting Fetch BSE-------------------------------------"
echo 
send_slack_exec=/home/pengine/prod/live_execs/send_slack_notification
YYYYMMDD=$1 ; 
DD=${YYYYMMDD:6:2} 
MM=${YYYYMMDD:4:2} 
YY=${YYYYMMDD:2:2}
YYYY=${YYYYMMDD:0:4}
MSTR=$(echo $(date -d $YYYYMMDD +%b) | awk '{print toupper($1)}') ;
#Mark it as failure for script by default

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
if [ $is_holiday = "1" ] ; then
   echo "BSE Holiday. Exiting...";
   exit;
fi 

FTP_DIR="/spare/local/files/BSEFTPFiles/$YYYY/$MM/$DD/"
rm -rf $FTP_DIR
mkdir -p $FTP_DIR
cd $FTP_DIR

[ $is_holiday == "2" ] || exit ; 

next_working_day=`/home/pengine/prod/live_execs/update_date $YYYYMMDD N A`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
while [[ $is_holiday = "1" ]] 
do
     next_working_day=`/home/pengine/prod/live_execs/update_date $next_working_day N A`
     is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
done

echo  "NextDay: $next_working_day";

# If today is a CD holiday, we look for the last working day for CD and copy the files from that date to Today's dated files
prev_cd_working_day=$YYYYMMDD
is_cd_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE_CD $prev_cd_working_day T`
while [ $is_cd_holiday = "1" ] 
do
     prev_cd_working_day=`/home/pengine/prod/live_execs/update_date $prev_cd_working_day P A`
     is_cd_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE_CD $prev_cd_working_day T`
done

if [ `ls $FTP_DIR | wc -l` -le 0 ] ; then
  fetch_cm_scrip ; 
  fetch_fo_scrip ;
  fetch_cm_bhavcopy ;
  fetch_fo_bhavcopy ;
  mkdir -p /spare/local/tradeinfo/BSE_Files/BhavCopy/cds/$MM$YY/
  # copy last CD working day's  cd files to today's date [ Only doing this if today is CD holiday ]
  if [ "$prev_cd_working_day" != "$YYYYMMDD" ]; then
    prev_YYYY=${prev_cd_working_day:0:4}
    prev_YY=${prev_cd_working_day:2:2}
    prev_MM=${prev_cd_working_day:4:2}
    prev_DD=${prev_cd_working_day:6:2}
    cp /spare/local/files/BSEFTPFiles/$prev_YYYY/$prev_MM/$prev_DD/BFX_CO${prev_DD}${prev_MM}${prev_YY}.csv $FTP_DIR/BFX_CO${DD}${MM}${YY}.csv
    cp /spare/local/files/BSEFTPFiles/$prev_YYYY/$prev_MM/$prev_DD/BFX_DP${prev_DD}${prev_MM}${prev_YY} $FTP_DIR/BFX_DP${DD}${MM}${YY}
    cp /spare/local/files/BSEFTPFiles/$prev_YYYY/$prev_MM/$prev_DD/BFX_SPD_CO${prev_DD}${prev_MM}${prev_YY}.csv $FTP_DIR/BFX_SPD_CO${DD}${MM}${YY}.csv
    cp /spare/local/tradeinfo/BSE_Files/BhavCopy/cds/${prev_MM}${prev_YY}/CurrencyBhavCopy_${prev_YYYY}${prev_MM}${prev_DD}.csv /spare/local/tradeinfo/BSE_Files/BhavCopy/cds/$MM$YY/CurrencyBhavCopy_${YYYY}${MM}${DD}.csv
  else
    fetch_cd_scrip ;
    fetch_cd_bhavcopy ; 
    mv CurrencyBhavCopy_${YYYY}${MM}${DD}.csv /spare/local/tradeinfo/BSE_Files/BhavCopy/cds/$MM$YY/
    #checking if blow files are of todays date if not then move the downloaded file to todays date
    ls | grep BFX_*
    [ -f "BFX_CO${DD}${MM}${YY}.csv" -a -s "BFX_CO${DD}${MM}${YY}.csv" -a -r "BFX_CO${DD}${MM}${YY}.csv" ] || mv $FTP_DIR/BFX_CO* $FTP_DIR/BFX_CO${DD}${MM}${YY}.csv
    [ -f "BFX_DP${DD}${MM}${YY}" -a -s "BFX_DP${DD}${MM}${YY}" -a -r "BFX_DP${DD}${MM}${YY}" ] || mv $FTP_DIR/BFX_DP* $FTP_DIR/BFX_DP${DD}${MM}${YY}
    [ -f "BFX_SPD_CO${DD}${MM}${YY}.csv" -a -s "BFX_SPD_CO${DD}${MM}${YY}.csv" -a -r "BFX_SPD_CO${DD}${MM}${YY}.csv" ] || mv $FTP_DIR/BFX_SPD_CO* $FTP_DIR/BFX_SPD_CO${DD}${MM}${YY}.csv
  fi
fi

echo "Updating BhavCopy files"
mkdir -p /spare/local/tradeinfo/BSE_Files/BhavCopy/fo/$MM$YY/ 

mv EQ${DD}${MM}${YY}.CSV /spare/local/tradeinfo/BSE_Files/Margin_Files/Exposure_Files/
#mv bhavcopy${DD}-${MM}-${YY}.csv /spare/local/tradeinfo/BSE_Files/BhavCopy/fo/$MM$YY/
cp /spare/local/tradeinfo/BSE_Files/BhavCopy/fo/0323/bhavcopy31-03-23.csv /spare/local/tradeinfo/BSE_Files/BhavCopy/fo/$MM$YY/bhavcopy${DD}-${MM}-${YY}.csv

sleep 20
verify_FTP_files "EQD_SPD_CO${DD}${MM}${YY}.csv"
verify_FTP_files "EQD_CO${DD}${MM}${YY}.csv"
verify_FTP_files "EQD_DP${DD}${MM}${YY}"
verify_FTP_files "BFX_CO${DD}${MM}${YY}.csv"
verify_FTP_files "BFX_SPD_CO${DD}${MM}${YY}.csv"
verify_FTP_files "BFX_DP${DD}${MM}${YY}"
verify_given_files "/spare/local/tradeinfo/BSE_Files/Margin_Files/Exposure_Files/EQ${DD}${MM}${YY}.CSV"
verify_given_files "/spare/local/tradeinfo/BSE_Files/BhavCopy/fo/$MM$YY/bhavcopy${DD}-${MM}-${YY}.csv"
verify_given_files "/spare/local/tradeinfo/BSE_Files/BhavCopy/cds/$MM$YY/CurrencyBhavCopy_${YYYY}${MM}${DD}.csv"

echo "CHECKED";


check_bhavcopy_expiry /spare/local/tradeinfo/BSE_Files/BhavCopy/fo/$MM$YY/bhavcopy${DD}-${MM}-${YY}.csv;

/home/dvctrader/.conda/envs/env/bin/python /home/dvctrader/important/BSE/download_daily_margin.py "${DD}${MM}${YYYY}"
cd /spare/local/tradeinfo/BSE_Files/Margin_Files/VAR_FILES/
/home/dvctrader/.conda/envs/env/bin/html2csv VarFile${DD}${MM}${YYYY}.xls -o VarFile${YYYYMMDD}.csv

echo "generating MD files"
cd /spare/local/tradeinfo/BSE_Files/BhavCopy/fo/$MM$YY/
cat bhavcopy${DD}-${MM}-${YY}.csv | tail -n +2 | awk -F"," '{ OFS=","; print "NORMAL",$1,$2,toupper($3),$4,$5,"-",$6,$7,$8,$9,$14,$15,$16,$17}' > ${DD}${MM}fo_0000.md
chgrp infra bhavcopy${DD}-${MM}-${YY}.csv 
chgrp infra ${DD}${MM}fo_0000.md

cd /spare/local/tradeinfo/BSE_Files/BhavCopy/cds/$MM$YY/
cat CurrencyBhavCopy_${YYYY}${MM}${DD}.csv | tail -n +2 | awk -F"," '{ OFS=","; print "NORMAL",$1,$2,toupper($3),$4,$5,"-",$6,$7,$8,$9,$14,$15,$16,$17}' > ${DD}${MM}cd_0000.md
#verify_given_files /spare/local/tradeinfo/BSE_Files/Margin_Files/VAR_FILES/VarFile20210626.xls
chgrp infra CurrencyBhavCopy_${YYYY}${MM}${DD}.csv
chgrp infra ${DD}${MM}cd_0000.md
echo "Updating REF files"
>/spare/local/tradeinfo/BSE_Files/RefData/bse_fo_$next_working_day"_contracts.txt"
>/spare/local/tradeinfo/BSE_Files/RefData/bse_cd_$next_working_day"_contracts.txt"
cat $FTP_DIR/SCRIP/SCRIP_${DD}${MM}${YY}.TXT | grep "|EQ|" | awk -F"|" '{sub(" ", "_"); print $1,0,"STK",$3,0,0,"XX","STK",0,0}'  > /spare/local/tradeinfo/BSE_Files/RefData/bse_eq_"$next_working_day"_contracts.txt
chgrp infra /spare/local/tradeinfo/BSE_Files/RefData/bse_cd_$next_working_day"_contracts.txt"
chgrp infra /spare/local/tradeinfo/BSE_Files/RefData/bse_fo_$next_working_day"_contracts.txt"
chgrp infra /spare/local/tradeinfo/BSE_Files/RefData/bse_eq_"$next_working_day"_contracts.txt

#awk -F 'NR==FNR{a[$1]=$6;b[$1]=$7; next} ($1 in a) {print $1,$2,$3,$4,$7,$8,$9,$54,a[$1],b[$1]}' EQD_DP250621 EQD_CO250621.csv
#tail -n +2 $FTP_DIR/EQD_CO${DD}${MM}${YY}.csv | awk -F',' '{print $1,$2,$3,$4,$7,$8,$9,$54,0,0}' > /spare/local/tradeinfo/BSE_Files/RefData/bse_fo_$next_working_day"_contracts.txt" 
awk -F',' 'NR==FNR{a[$1]=$6;b[$1]=$7; next} ($1 in a) {x=$9;if($9== ""){x="XX"};print $1,$2,$3,$4,$7,$8,x,$54,a[$1],b[$1]}' $FTP_DIR/EQD_DP${DD}${MM}${YY} $FTP_DIR/EQD_CO${DD}${MM}${YY}.csv > /spare/local/tradeinfo/BSE_Files/RefData/bse_fo_$next_working_day"_contracts.txt"
#awk -F',' 'NR==FNR{a[$1]=$6;b[$1]=$7; next} ($1 in a) {print $1,$2,$3,$4,$7,$8,$9,$54,a[$1],b[$1]}' BFX_DP250621 BFX_CO250621.csv 
#tail -n +2 BFX_CO250621.csv | awk -F',' '{print $1,$2,$3,$4,$7,$8,$9,$54,0,0}' > /spare/local/tradeinfo/BSE_Files/RefData/bse_cd_$next_working_day"_contracts.txt"
awk -F',' 'NR==FNR{a[$1]=$6;b[$1]=$7; next} ($1 in a) {x=$9;if($9== ""){x="XX"};print $1,$2,$3,$4,$7,$8,x,$54,a[$1],b[$1]}' $FTP_DIR/BFX_DP${DD}${MM}${YY} $FTP_DIR/BFX_CO${DD}${MM}${YY}.csv > /spare/local/tradeinfo/BSE_Files/RefData/bse_cd_$next_working_day"_contracts.txt"

echo "Updating LOT file"
#LOTSIZE FILE
grep FUT $FTP_DIR/EQD_CO${DD}${MM}${YY}.csv | cut -d',' -f1,5,7,31 > /spare/local/tradeinfo/BSE_Files/Lotsizes/fo_bse_lot_$next_working_day
chgrp infra /spare/local/tradeinfo/BSE_Files/Lotsizes/fo_bse_lot_$next_working_day

get_expiry_date;

echo "Expiry : $EXPIRY";
N_MM=$MM
N_YY=$YY
N_YYYY=$YYYY
if [ $YYYYMMDD -ge $EXPIRY ] ; then
  NEXT_MONTH=`date -d "$YYYYMMDD + 1 month - 10 day" +"%m%Y"`
  N_MM=${NEXT_MONTH:0:2}
  N_YY=${NEXT_MONTH:4:2}
  N_YYYY=${NEXT_MONTH:2:4}
#  fetch_mkt_lots ;
  echo "NextMonth: $N_MM , $N_YY"
fi

echo "Updating SecBan file"
cd /spare/local/tradeinfo/BSE_Files/SecuritiesUnderBan/
wget -t 2 --timeout=20 --referer https://www.bseindia.com --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www.bseindia.com/Download/Derivative/MWPL/BannedScrips-${DD}${MM}${YYYY}.zip
unzip BannedScrips-${DD}${MM}${YYYY}.zip
rm BannedScrips-${DD}${MM}${YYYY}.zip
tail -n +2 /spare/local/tradeinfo/BSE_Files/SecuritiesUnderBan/BannedScrips-${DD}${MM}${YYYY}.csv | cut -d',' -f4 > /spare/local/tradeinfo/BSE_Files/SecuritiesUnderBan/fo_secban_${next_working_day}.csv
verify_given_files "/spare/local/tradeinfo/BSE_Files/SecuritiesUnderBan/BannedScrips-${DD}${MM}${YYYY}.csv"
chgrp infra "/spare/local/tradeinfo/BSE_Files/SecuritiesUnderBan/BannedScrips-${DD}${MM}${YYYY}.csv"
chgrp infra /spare/local/tradeinfo/BSE_Files/SecuritiesUnderBan/fo_secban_${next_working_day}.csv

echo "Updating Span file"
year=${next_working_day:0:4}
month=`date -d $next_working_day +"%b"`
next_day=${next_working_day:6:2}
cd /spare/local/tradeinfo/BSE_Files/Margin_Files/Span_Files/
wget -t 2 --timeout=20 --referer https://www.bseindia.com --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www.bseindia.com/bsedata/Risk_Automate/SPN/BSERISK${YYYY}${MM}${DD}-FINAL.ZIP
unzip -o BSERISK${YYYY}${MM}${DD}-FINAL.ZIP
rm BSERISK${YYYY}${MM}${DD}-FINAL.ZIP
chmod 666 BSERISK${YYYY}${MM}${DD}-FINAL.spn
verify_given_files /spare/local/tradeinfo/BSE_Files/Margin_Files/Span_Files/BSERISK${YYYY}${MM}${DD}-FINAL.spn
chgrp infra /spare/local/tradeinfo/BSE_Files/Margin_Files/Span_Files/BSERISK${YYYY}${MM}${DD}-FINAL.spn

echo "Updating SecurityMargin files"
cd /spare/local/tradeinfo/BSE_Files/Margin_Files/Exposure_Files
awk -F"," 'NR==FNR{a[$1]=$10; next} ($1 in a){print $1"|"(a[$1]*$8)/100}' /spare/local/tradeinfo/BSE_Files/Margin_Files/VAR_FILES/VarFile${YYYYMMDD}.csv EQ${DD}${MM}${YY}.CSV  > /spare/local/tradeinfo/BSE_Files/SecuritiesMarginFiles/security_margin_"$next_working_day".txt_tmp

awk -F"|" 'NR==FNR{a[$1]=$3; next} ($1 in a){print "BSE_"a[$1]" "$2}'  $FTP_DIR/SCRIP/SCRIP_${DD}${MM}${YY}.TXT /spare/local/tradeinfo/BSE_Files/SecuritiesMarginFiles/security_margin_"$next_working_day".txt_tmp > /spare/local/tradeinfo/BSE_Files/SecuritiesMarginFiles/security_margin_"$next_working_day".txt

rm /spare/local/tradeinfo/BSE_Files/SecuritiesMarginFiles/security_margin_"$next_working_day".txt_tmp
verify_given_files /spare/local/tradeinfo/BSE_Files/SecuritiesMarginFiles/security_margin_"$next_working_day".txt
echo "SM File generated"
chgrp infra /spare/local/tradeinfo/BSE_Files/SecuritiesMarginFiles/security_margin_"$next_working_day".txt

echo "Product less then Rs100 file"
next_MMYYYY=${next_working_day:4:2}${next_working_day:0:4}
if [ $next_MMYYYY -ne ${MM}${YYYY} ]; then
  awk -F',' '{if ($8 <= 100) print $1}' EQ${DD}${MM}${YY}.CSV > /tmp/exchange_id_bse
  awk '(NR==FNR) {secid[$1]=1; next;} { if ($1 in secid) print "BSE_"$4 }' /tmp/exchange_id_bse /spare/local/tradeinfo/BSE_Files/RefData/bse_eq_"$next_working_day"_contracts.txt | sort | uniq > /spare/local/tradeinfo/BSE_Files/SecuritiesPriceLess100/EQ_PRICE_LESS_100_${YYYYMMDD}
else
  prev_work_day=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P W`
  cp /spare/local/tradeinfo/BSE_Files/SecuritiesPriceLess100/EQ_PRICE_LESS_100_${prev_work_day} /spare/local/tradeinfo/BSE_Files/SecuritiesPriceLess100/EQ_PRICE_LESS_100_${YYYYMMDD}
fi

echo "Updating groupAB product file"
groupAB_file="/spare/local/tradeinfo/BSE_Files/RefData/bse_groupAB_symbol_bhavcopy_${next_working_day}"
>$groupAB_file
while IFS= read -r line;
do
   sc_group=`echo "$line"| awk -F, '{print $3}'`
   if [ $sc_group = 'A' ] || [ $sc_group = 'B' ]; then
      stk_code=`echo "$line" | awk -F, '{print $1}'`
      sc_symbol=`grep $stk_code "/spare/local/tradeinfo/BSE_Files/RefData/bse_eq_${next_working_day}_contracts.txt" | awk '{print $4}'`
      if [ ! -z "$sc_symbol" ]; then
         echo  "$sc_symbol, $line" >>"$groupAB_file"
      fi
   fi
done <"/spare/local/tradeinfo/BSE_Files/Margin_Files/Exposure_Files/EQ${DD}${MM}${YY}.CSV"

#/home/dvctrader/important/sync_bse_trade_files.sh
rsync -ravz --timeout=60 /spare/local/tradeinfo/BSE_Files 10.23.5.66:/spare/local/tradeinfo
rsync -ravz --timeout=60 /spare/local/tradeinfo/BSE_Files 192.168.132.11:/spare/local/tradeinfo
rsync -ravz --timeout=60 /spare/local/tradeinfo/BSE_Files 192.168.132.12:/spare/local/tradeinfo
rsync -ravz --timeout=60 /spare/local/tradeinfo/BSE_Files 10.23.5.50:/spare/local/tradeinfo
rsync -ravz --timeout=60 /spare/local/tradeinfo/BSE_Files 10.23.5.22:/spare/local/tradeinfo

rsync -ravz /spare/local/files/BSEFTPFiles 10.23.5.66:/spare/local/files
rsync -ravz /spare/local/files/BSEFTPFiles 10.23.5.50:/spare/local/files
rsync -ravz /spare/local/files/BSEFTPFiles 10.23.5.22:/spare/local/files
rsync -ravz /spare/local/files/BSEFTPFiles 192.168.132.11:/spare/local/files
rsync -ravz /spare/local/files/BSEFTPFiles 192.168.132.12:/spare/local/files
#verify_given_file /spare/local/tradeinfo/BSE_Files/Margin_Files/Exposure_Files/exposure_margin_rates.$YYYYMMDD
#verify_given_file /spare/local/tradeinfo/BSE_Files/ContractFiles/bse_contracts.$next_working_day
#verify_given_file /spare/local/tradeinfo/BSE_Files/datasource_exchsymbol.txt

