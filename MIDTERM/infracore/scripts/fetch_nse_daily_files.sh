#!/bin/bash

print_usage_and_exit () {
    echo "$0 YYYYMMDD" ;  
    exit ; 
} 

FTP_HOST='ftp.connect2nse.com'

fetch_mkt_lots () {

ftp -n $FTP_HOST <<SCRIPT
  user "FAOGUEST" "FAOGUEST" 
  cd faoftp 
  cd faocommon
  binary 
  get fo_mktlots.csv
  quit 
SCRIPT

cp $FTP_DIR/fo_mktlots.csv /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv"

}

check_bhavcopy_expiry () {
least_expiry=`cat $1 | cut -d',' -f4 | cut -d' ' -f3 | sort | head -1`
    if [ $least_expiry -lt $YYYY ]; then
#      printf "Older expirires present in bhavcopy file" | /bin/mail -s "NSEDailyFilesGenerationFailure - $1" -r "ravi.parikh@tworoads.co.in" "nseall@tworoads.co.in" ;
      exit;
    fi
}


fetch_stock_security () {

ftp -n $FTP_HOST <<SCRIPT
  user "ftpguest" "FTPGUEST" 
  cd Common
  cd NTNEAT
  binary 
  get security.gz
  quit 
SCRIPT

zgrep "|EQ|" $FTP_DIR/security.gz | awk -F"|" '{print $1,0,"STK",$2,0,0,"XX","STK",0,0}' > /spare/local/tradeinfo/NSE_Files/RefData/nse_eq_"$next_working_day"_contracts.txt 
}

fetch_fo_ref_contracts () {

ftp -n $FTP_HOST <<SCRIPT
  user "FAOGUEST" "FAOGUEST" 
  passive
  cd faoftp 
  cd faocommon
  binary 
  get security.gz 
  get contract.gz
  get fo_participant.gz
  get spd_contract.gz 
  quit 
SCRIPT

}

fetch_cd_ref_contracts () {

ftp -n $FTP_HOST <<SCRIPT 
  user "CDSGUEST" "CDSGUEST"
  cd cdsftp
  cd cdscommon
  binary 
  get cd_contract.gz
  get cd_participant.gz
  get cd_spd_contract.gz
  quit
SCRIPT

} 

fetch_fo_settl_bhavcopy () {
  wget --referer https://www.nseindia.com/products/content/equities/equities/homepage_eq.htm --user-agent="Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www.nseindia.com/content/historical/DERIVATIVES/"$YYYY"/"$MSTR"/fo"$DD"$MSTR"$YYYY"bhav.csv.zip
  unzip fo"$DD"$MSTR"$YYYY"bhav.csv.zip
  rm fo"$DD"$MSTR"$YYYY"bhav.csv.zip
}


fetch_fo_bhavcopy () {

ftp -n $FTP_HOST <<SCRIPT 
  user "FAOGUEST" "FAOGUEST" 
  cd faoftp 
  cd faocommon
  cd Bhavcopy 
  binary 
  get $DD$MM"0000.md"
  quit 
SCRIPT

if [ -e $DD$MM"0000".md ]; then
  mv $DD$MM"0000".md $DD$MM"fo_0000".md
else
echo "Getting FO Bhavcopy from website.."
#Month str like FEB, MAR etc.
  MSTR=$(echo $(date -d $YYYYMMDD +%b) | awk '{print toupper($1)}') ;

#if file dowsnot exist on FTP, download from website
  wget --no-check-certificate --user-agent="Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www1.nseindia.com/content/historical/DERIVATIVES/"$YYYY"/"$MSTR"/fo"$DD"$MSTR"$YYYY"bhav.csv.zip ;

#Unzip and convert to FTP bhavcopy format
  unzip -o fo"$DD"$MSTR"$YYYY"bhav.csv.zip ;
  cat fo"$DD"$MSTR"$YYYY"bhav.csv | tail -n +2 | awk -F"," '{ OFS=","; gsub(/-/," ",$3); print "NORMAL",$1,$2,toupper($3),$4,$5,"-",$6,$7,$8,$9,$11,$12,$13,$14}' > $DD$MM"fo_0000".md ;
fi

chown dvctrader:infra $DD$MM"fo_0000".md
} 

fetch_cd_bhavcopy () {
ftp -n $FTP_HOST <<SCRIPT
  user "CDSGUEST" "CDSGUEST"  
  cd cdsftp
  cd cdscommon
  cd Bhavcopy 
  binary 
  get $DD$MM"0000.md"
SCRIPT

if [ -e $DD$MM"0000".md ];then 
  mv $DD$MM"0000".md $DD$MM"cd_0000".md ;
else

echo "Getting CD Bhavcopy from website.."
#Get from website
  wget --no-check-certificate --user-agent="Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0"  https://www1.nseindia.com/archives/cd/bhav/CD_Bhavcopy"$DD$MM$YY".zip
  wget --no-check-certificate --user-agent="Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0"  https://www1.nseindia.com/archives/ird/sett/CDSett_prce_IRF_"$DD$MM$YYYY".csv

#Unzip CD Bhavcopy, It gives two files for FO and OP
  unzip -o CD_Bhavcopy"$DD$MM$YY".zip

#convert the FO CD file
  cat CD_NSE_FO"$DD$MM$YY".csv | tail -n+2 | awk -F"," '{OFS=","; s1=substr($1,1,6); s2=substr($1,7,6); s3=substr($1,13,12); gsub(/-/," ",s3); print "NORMAL",s1,s2,s3," "," ",$2,$3,$4,$5,$6,$11,"-","-","-"}' > $DD$MM"cd_0000".md

#convert the OP CD file
  cat CD_NSE_OP"$DD$MM$YY".csv | tail -n+2 | awk -F"," '{OFS=","; s1=substr($1,1,6); s2=substr($1,7,6); s3=substr($1,13,11); s4=substr($1,24,2); s5=substr($1,26); gsub(/-/," ",s3); print "NORMAL",s1,s2,s3,s5,s4,$2,$3,$4,$5,$6,$11,"-","-","-"}' >> $DD$MM"cd_0000".md

#Convert Bond Futures file
  cat CDSett_prce_IRF_"$DDMM$YYYY".csv | tail -n+2 | awk -F"," '{OFS=","; gsub(/-/," ",$4); print "NORMAL",$2,$3,$4," "," ","0",$5,$5,$5,$5,"-","-","-"}' >> $DD$MM"cd_0000".md

fi

chown dvctrader:infra $DD$MM"cd_0000".md
}

verify_given_file () {

  GIVEN_FILE=$1;
#    [ -f $GIVEN_FILE -a -s $GIVEN_FILE -a -r $GIVEN_FILE ] || printf "Contract File not present for today" | /bin/mail -s "NSEDailyFilesGenerationFailure - $GIVEN_FILE" -r "ravi.parikh@tworoads.co.in" "nseall@tworoads.co.in" ;
    [ -f $GIVEN_FILE -a -s $GIVEN_FILE -a -r $GIVEN_FILE ] || exit ;

}
verify_files () {

  FILENAME=$FTP_DIR/$1 ;  

#  [ -f $FILENAME -a -s $FILENAME -a -r $FILENAME ] || printf "NSE_DAILY_FILE_GENERATION_FAILURE" | /bin/mail -s "NSEDailyFilesGenerationFailure - $FILENAME" -r "ravi.parikh@tworoads.co.in" "nseall@tworoads.co.in" ; 
  [ -f $FILENAME -a -s $FILENAME -a -r $FILENAME ] || exit ; 

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

send_slack_exec=/home/pengine/prod/live_execs/send_slack_notification
YYYYMMDD=$1 ; 
DD=${YYYYMMDD:6:2} 
MM=${YYYYMMDD:4:2} 
YY=${YYYYMMDD:2:2}
YYYY=${YYYYMMDD:0:4}
MSTR=$(echo $(date -d $YYYYMMDD +%b) | awk '{print toupper($1)}') ;
#Mark it as failure for script by default
STATUS_FILE=/tmp/NSEDailyReportsErrors/NSEDAILYREPORTSFAILED_$YYYYMMDD ;  
touch $STATUS_FILE ;
date +"%s" > $STATUS_FILE ;

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi 

FTP_DIR="/spare/local/files/NSEFTPFiles/$YYYY/$MM/$DD/"
mkdir -p $FTP_DIR
cd $FTP_DIR

#[ $is_holiday == "2" ] || printf "NSE_DAILY_FILE_GENERATION_FAILURE" | /bin/mail -s "NSEDailyFilesGenerationFailure - NSE_Holidays" -r "ravi.parikh@tworoads.co.in" "nseall@tworoads.co.in" ; 
[ $is_holiday == "2" ] || exit ; 

next_working_day=`/home/pengine/prod/live_execs/update_date $YYYYMMDD N W`

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
while [ $is_holiday = "1" ] 
do
     next_working_day=`/home/pengine/prod/live_execs/update_date $next_working_day N W`
     is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
done

echo  "NextDay: $next_working_day";

if [ `ls $FTP_DIR | wc -l` -le 0 ] ; then
	fetch_fo_ref_contracts ; 
	fetch_cd_ref_contracts ;
	fetch_fo_bhavcopy ;
	fetch_cd_bhavcopy ; 
        fetch_fo_settl_bhavcopy ;
fi

fetch_stock_security;

# If today is a CD holiday, we look for the last working day for CD and copy the files from that date to Today's dated files
prev_cd_working_day=$YYYYMMDD
is_cd_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE_CD $prev_cd_working_day T`
while [ $is_cd_holiday = "1" ] 
do
     prev_cd_working_day=`/home/pengine/prod/live_execs/update_date $prev_cd_working_day P W`
     is_cd_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE_CD $prev_cd_working_day T`
done
# copy last CD working day's  cd files to today's date [ Only doing this if today is CD holiday ]
if [ "$prev_cd_working_day" != "$YYYYMMDD" ]; then
  prev_YYYY=${prev_cd_working_day:0:4}
  prev_MM=${prev_cd_working_day:4:2}
  prev_DD=${prev_cd_working_day:6:2}
  cp /spare/local/files/NSEFTPFiles/$prev_YYYY/$prev_MM/$prev_DD/*cd_* $FTP_DIR/
  mv $FTP_DIR/$prev_DD$prev_MM"cd_0000.md" $FTP_DIR/$DD$MM"cd_0000.md"
fi

gunzip -df *
unzip -o * 

verify_files "contract"
verify_files "security"
verify_files "fo_participant"
verify_files "spd_contract"
verify_files "cd_contract"
verify_files "cd_participant"
verify_files "cd_spd_contract"
verify_files $DD$MM"fo_0000".md
verify_files $DD$MM"cd_0000".md

mkdir -p /spare/local/tradeinfo/NSE_Files/BhavCopy/cds/$MM$YY/
mkdir -p /spare/local/tradeinfo/NSE_Files/BhavCopy/fo/$MM$YY/ 

check_bhavcopy_expiry $FTP_DIR/$DD$MM"fo_0000".md;
check_bhavcopy_expiry $FTP_DIR/$DD$MM"cd_0000".md;
 
cp $FTP_DIR/$DD$MM"fo_0000".md /spare/local/tradeinfo/NSE_Files/BhavCopy/fo/$MM$YY/
cp $FTP_DIR/$DD$MM"cd_0000".md /spare/local/tradeinfo/NSE_Files/BhavCopy/cds/$MM$YY/
mv $FTP_DIR/fo"$DD"$MSTR"$YYYY"bhav.csv /spare/local/tradeinfo/NSE_Files/BhavCopy/fo/$MM$YY/

rm -rf /spare/local/tradeinfo/NSE_Files/RefData/nse_fo_$next_working_day"_contracts.txt"
rm -rf /spare/local/tradeinfo/NSE_Files/RefData/nse_cd_$next_working_day"_contracts.txt"

for i in FUTSTK OPTSTK FUTIDX OPTIDX FUTINT FUTIVX UNDINT ; do cat $FTP_DIR/contract | awk -F"|" '{print $1,$2,$3,$4,$7,$8,$9,$54,$43,$44}' | grep "$i" >> /spare/local/tradeinfo/NSE_Files/RefData/nse_fo_$next_working_day"_contracts.txt" ; done
for i in FUTCUR FUTIRC FUTIRT INDEX OPTCUR UNDCUR UNDIRC UNDIRD UNDIRT; do cat $FTP_DIR/cd_contract | awk -F"|" '{print $1,$2,$3,$4,$7,$8,$9,$54,$43,$44}' | grep "$i" >> /spare/local/tradeinfo/NSE_Files/RefData/nse_cd_$next_working_day"_contracts.txt" ; done 

get_expiry_date;

echo "Expiry : $EXPIRY";
N_MM=$MM
N_YY=$YY
if [ $YYYYMMDD -ge $EXPIRY ] ; then
  NEXT_MONTH=`date -d "$YYYYMMDD + 1 month - 10 day" +"%m%y"`
  N_MM=${NEXT_MONTH:0:2}
  N_YY=${NEXT_MONTH:2:2}
  fetch_mkt_lots ;
  echo "NextMonth: $N_MM , $N_YY"
fi

if [ $YYYYMMDD -eq $EXPIRY ] ; then
/home/pengine/prod/live_scripts/NSE_File_Generator.pl /spare/local/tradeinfo/NSE_Files/BhavCopy/fo/$MM$YY/$DD$MM"fo_0000".md /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv"  /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_$N_MM$N_YY".csv" /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_working_day C /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt $EXPIRY

/home/pengine/prod/live_scripts/NSE_File_Generator.pl /spare/local/tradeinfo/NSE_Files/BhavCopy/cds/$MM$YY/$DD$MM"cd_0000".md /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv"  /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_$N_MM$N_YY".csv" /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_working_day A /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt $EXPIRY
else
/home/pengine/prod/live_scripts/NSE_File_Generator.pl /spare/local/tradeinfo/NSE_Files/BhavCopy/fo/$MM$YY/$DD$MM"fo_0000".md /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv"  /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_$N_MM$N_YY".csv" /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_working_day C /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt

/home/pengine/prod/live_scripts/NSE_File_Generator.pl /spare/local/tradeinfo/NSE_Files/BhavCopy/cds/$MM$YY/$DD$MM"cd_0000".md /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv"  /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_$N_MM$N_YY".csv" /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_working_day A /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt
fi

cd /spare/local/tradeinfo/NSE_Files/SecuritiesUnderBan/
rm fo_secban.csv
wget --no-check-certificate --user-agent="Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www.nseindia.com/content/fo/fo_secban.csv
grep -v "Securities" fo_secban.csv | awk -F"," '{print $2}' > fo_secban_$next_working_day".csv"
#Fixing invisible character below
cat fo_secban_$next_working_day".csv" | sed 's/\r$//' > fo_secban_$next_working_day".csv_temp"
mv fo_secban_$next_working_day".csv_temp" fo_secban_$next_working_day".csv"

year=${next_working_day:0:4}
month=`date -d $next_working_day +"%b"`
next_day=${next_working_day:6:2}

if [ "`grep -i "$next_day.*$month.*$year" fo_secban.csv | wc -l `" -eq "0" ]; then
   error_msg="SecBan File for $next_working_day is not latest. Please download the file later"
   $send_slack_exec prod-issues DATA "$error_msg"
fi

cd /spare/local/tradeinfo/NSE_Files/Margin_Files/Span_Files/
wget --no-check-certificate --user-agent="Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www.nseindia.com/archives/nsccl/span/nsccl.$YYYYMMDD.s_1.zip
unzip -o nsccl.$YYYYMMDD.s_1.zip
rm nsccl.$YYYYMMDD.s_1.zip
chmod 666 nsccl.$YYYYMMDD.s_1.spn
scp -p nsccl.$YYYYMMDD.s_1.spn dvcinfra@10.23.74.41:/data1/apps/data/NSEMidTerm/MarginFiles/SpanFiles
prev_day=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P W 1`

>"/spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles/security_margin_"$next_working_day".txt"
month_name=`date -d $YYYYMMDD '+%b' | awk '{print toupper($0)}'`
wget --no-check-certificate --user-agent="Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www.nseindia.com/archives/nsccl/var/C_VAR1_"$DD$MM$YYYY"_6.DAT
wget --no-check-certificate --referer https://www.nseindia.com/products/content/equities/equities/homepage_eq.html --user-agent="Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www.nseindia.com/content/historical/EQUITIES/"$YYYY"/"$month_name"/cm"$DD""$month_name""$YYYY"bhav.csv.zip

if [ ! -f cm"$DD""$month_name""$YYYY"bhav.csv.zip]; then
  error_msg="Unable to download Bhavcopy file for $YYYYMMDD. Please download the file manually"
  $send_slack_exec prod-issues DATA "$error_msg"
fi

unzip cm"$DD""$month_name""$YYYY"bhav.csv.zip
rm cm"$DD""$month_name""$YYYY"bhav.csv.zip

awk -F"," '$3 == "EQ" {print $2","$10}'  C_VAR1_"$DD$MM$YYYY"_6.DAT > cm_loss_rate_"$DD$MM$YYYY"
awk -F"," '$2 == "EQ" {print $1","$6}'  cm"$DD""$month_name""$YYYY"bhav.csv > cm_closing_price_"$DD$MM$YYYY"

awk -F "," 'FNR==NR{a[$1]=$2;next}($1 in a){print $1,a[$1],$2}' OFS="," cm_loss_rate_"$DD$MM$YYYY" cm_closing_price_"$DD$MM$YYYY" | awk -F "," '{print $1" "($2*$3/100)""}' > cm_var_"$DD$MM$YYYY"

cp cm_var_"$DD$MM$YYYY" cm_var_"$DD$MM$YYYY"_copy
awk 'BEGIN{OFS=" "}$1="NSE_"$1' cm_var_"$DD$MM$YYYY"_copy > cm_var_"$DD$MM$YYYY"
cat cm_var_"$DD$MM$YYYY" >> /spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles/security_margin_"$next_working_day".txt

rm cm_var_"$DD$MM$YYYY" cm_var_"$DD$MM$YYYY"_copy cm_loss_rate_"$DD$MM$YYYY" cm_closing_price_"$DD$MM$YYYY" cm"$DD""$month_name""$YYYY"bhav.csv C_VAR1_"$DD$MM$YYYY"_6.DAT

cd /spare/local/tradeinfo/NSE_Files/Margin_Files/Span_Files/ ;
find -type f -mtime +2 -exec rm -f {} \; 

verify_given_file /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_working_day
verify_given_file /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt

#Update midterm_db with Lotsizes for next trading day
cp /spare/local/tradeinfo/NSE_Files/midterm_db /home/dvctrader/trash/midterm_db
/apps/anaconda/anaconda3/bin/python /home/dvctrader/infracore/scripts/Update_MidTermDB_Lotsizes.py $next_working_day /home/dvctrader/trash/midterm_db
mv /home/dvctrader/trash/midterm_db /spare/local/tradeinfo/NSE_Files/midterm_db

#Update Fo Next trading per symbol unit margins
declare -A hashmap
margin_exec_="/home/pengine/prod/live_execs/calc_nse_span"
exch_sym_exec_="/home/pengine/prod/live_execs/get_exchange_symbol"
symbols="/home/pengine/prod/live_configs/nse_midterm_shortcodes"
ds_sym_file="/spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt"
temp_shc_file_="/home/dvctrader/trash/temp_shcs"
contract_file_="/spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts."$next_working_day
#For options
for i in `cat $symbols`; do
  if [[ "`echo $i | cut -d'_' -f3 | grep FUT`" == "" ]]; then
    stock_=`echo $i | cut -d'_' -f2`
    if [[ "`echo ${hashmap[$stock_]}`" == "" ]]; then
       ex_sym=`$exch_sym_exec_ $i $next_working_day`
       ds_sym=`grep $ex_sym $ds_sym_file | awk 'BEGIN{FS=" "}{print $2}'`
       echo -e "$ds_sym\t-1" >$temp_shc_file_
       hashmap[$stock_]=`$margin_exec_ $YYYYMMDD $temp_shc_file_ | awk 'BEGIN{FS="_"}{print $1+$2}'`
    fi
    margin=`echo "${hashmap[$stock_]}"`
    echo $i" "$margin >> "/spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles/security_margin_"$next_working_day".txt"
  fi
done


#For all futures from Contract File
declare -A expmap
expiries=(`cat $contract_file_ | grep "IDXFUT" | awk 'BEGIN{FS="\t"}{print $6}' | sort -u`)
for i in `seq 0 $((${#expiries[@]}-1))`; do  expmap[${expiries[i]}]=$i ; done

for i in `cat $contract_file_ | egrep "IDXFUT|STKFUT" | awk 'BEGIN{FS="\t"; OFS=","}{print $2,$6}'`; do    
    stk=`echo $i | cut -d',' -f1`
    exp=`echo $i | cut -d',' -f2`
    ds_sym="NSE_"$stk"_FUT_"$exp
    shc="NSE_"$stk"_FUT"${expmap[$exp]}
    echo -e "$ds_sym\t-1" > $temp_shc_file_
    margin=`$margin_exec_ $YYYYMMDD $temp_shc_file_ | awk 'BEGIN{FS="_"}{print $1+$2}'`
    echo $shc" "$margin >> "/spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles/security_margin_"$next_working_day".txt"
done


find /spare/local/tradeinfo/NSE_Files/DataExchFile -type f -mtime +7 -exec rm -f {} \;

rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.27.2:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.27.3:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.27.4:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.27.5:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.27.6:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.27.7:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.27.8:/spare/local/tradeinfo --delete-after
#scp /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt dvctrader@10.23.115.62:/spare/local/tradeinfo/NSE_Files/
#scp /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts."$next_working_day" dvcinfra@10.23.115.62:/spare/local/tradeinfo/NSE_Files/ContractFiles/
#scp /spare/local/tradeinfo/NSE_Files/RefData/nse_fo_"$next_working_day"_contracts.txt dvcinfra@10.23.115.62:/spare/local/tradeinfo/NSE_Files/RefData/
#scp /spare/local/tradeinfo/NSE_Files/RefData/nse_cd_"$next_working_day"_contracts.txt dvcinfra@10.23.115.62:/spare/local/tradeinfo/NSE_Files/RefData/
#scp /spare/local/tradeinfo/NSE_Files/RefData/nse_eq_"$next_working_day"_contracts.txt dvcinfra@10.23.115.62:/spare/local/tradeinfo/NSE_Files/RefData/

rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.27.9:spare/local/tradeinfo --delete-after
#scp /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt dvctrader@10.23.115.61:/spare/local/tradeinfo/NSE_Files/
#scp /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts."$next_working_day" dvcinfra@10.23.115.61:/spare/local/tradeinfo/NSE_Files/ContractFiles/
#scp /spare/local/tradeinfo/NSE_Files/RefData/nse_fo_"$next_working_day"_contracts.txt dvcinfra@10.23.115.61:/spare/local/tradeinfo/NSE_Files/RefData/
#scp /spare/local/tradeinfo/NSE_Files/RefData/nse_cd_"$next_working_day"_contracts.txt dvcinfra@10.23.115.61:/spare/local/tradeinfo/NSE_Files/RefData/
#scp /spare/local/tradeinfo/NSE_Files/RefData/nse_eq_"$next_working_day"_contracts.txt dvcinfra@10.23.115.61:/spare/local/tradeinfo/NSE_Files/RefData/

#scp /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt dvctrader@10.23.27.2:/spare/local/tradeinfo/NSE_Files/
#scp /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts."$next_working_day" dvcinfra@10.23.27.2:/spare/local/tradeinfo/NSE_Files/ContractFiles/
#scp /spare/local/tradeinfo/NSE_Files/RefData/nse_fo_"$next_working_day"_contracts.txt dvcinfra@10.23.27.2:/spare/local/tradeinfo/NSE_Files/RefData/
#scp /spare/local/tradeinfo/NSE_Files/RefData/nse_cd_"$next_working_day"_contracts.txt dvcinfra@10.23.27.2:/spare/local/tradeinfo/NSE_Files/RefData/
#scp /spare/local/tradeinfo/NSE_Files/RefData/nse_eq_"$next_working_day"_contracts.txt dvcinfra@10.23.27.2:/spare/local/tradeinfo/NSE_Files/RefData/

#scp /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt dvctrader@10.23.27.3:/spare/local/tradeinfo/NSE_Files/
#scp /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts."$next_working_day" dvcinfra@10.23.27.3:/spare/local/tradeinfo/NSE_Files/ContractFiles/
#scp /spare/local/tradeinfo/NSE_Files/RefData/nse_fo_"$next_working_day"_contracts.txt dvcinfra@10.23.27.3:/spare/local/tradeinfo/NSE_Files/RefData/
#scp /spare/local/tradeinfo/NSE_Files/RefData/nse_cd_"$next_working_day"_contracts.txt dvcinfra@10.23.27.3:/spare/local/tradeinfo/NSE_Files/RefData/
#scp /spare/local/tradeinfo/NSE_Files/RefData/nse_eq_"$next_working_day"_contracts.txt dvcinfra@10.23.27.3:/spare/local/tradeinfo/NSE_Files/RefData/


scp /spare/local/files/stocks.db dvcinfra@10.23.115.61:/spare/local/files/
scp /spare/local/files/stocks.db dvcinfra@10.23.115.62:/spare/local/files/
scp /spare/local/files/stocks.db dvcinfra@10.23.115.63:/spare/local/files/
scp /spare/local/files/stocks.db dvcinfra@10.23.115.64:/spare/local/files/
scp /spare/local/files/stocks.db dvcinfra@10.23.115.65:/spare/local/files/

scp /spare/local/files/stocks.db dvcinfra@10.23.27.3:/spare/local/files/
scp /spare/local/files/stocks.db dvcinfra@10.23.27.2:/spare/local/files/
scp /spare/local/files/stocks.db dvcinfra@10.23.27.4:/spare/local/files/
scp /spare/local/files/stocks.db dvcinfra@10.23.27.5:/spare/local/files/
scp /spare/local/files/stocks.db dvcinfra@10.23.27.6:/spare/local/files/

scp /spare/local/files/stocks.db dvcinfra@10.23.26.51:/spare/local/files/
scp /spare/local/files/stocks.db dvcinfra@10.23.26.52:/spare/local/files/
#Mark it as successful run of the script
rm -f $STATUS_FILE ;

host_name=`hostname`
all_server_cfg_file="/spare/local/tradeinfo/all_servers.cfg"

host_ip=""
if [[ ${host_name:0:6} == "sdv-ny" ]]; then
  if [[ ${host_name:8:5} == "srv11" ]]; then
    host_ip="10.23.74.51"
  elif [[ ${host_name:8:5} == "srv12" ]]; then
    host_ip="10.23.74.52"
  elif [[ ${host_name:8:5} == "srv13" ]]; then
    host_ip="10.23.74.53"
  elif [[ ${host_name:8:5} == "srv14" ]]; then
    host_ip="10.23.74.54"
  else
    host_ip="10.23.74.55"
  fi
fi

rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 54.208.92.178:/mnt/sdf/spare_local_tradeinfo  --delete-after

for server_ip in `grep -vE "#|$host_ip" $all_server_cfg_file`;
do
  rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files $server_ip:/spare/local/tradeinfo --delete-after
done

