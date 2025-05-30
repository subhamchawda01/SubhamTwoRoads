#!/bin/bash

print_usage_and_exit () {
    echo "$0 YYYYMMDD" ;  
    exit ; 
} 

FTP_HOST='ftp.connect2nse.com'

fetch_mkt_lots () {

ftp -np $FTP_HOST <<SCRIPT
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
      printf "Older expirires present in bhavcopy file" | /bin/mail -s "NSEDailyFilesGenerationFailure - $1" -r $HOSTNAME raghunandan.sharma@tworoads-trading.co.in "sanjeev.kumar@tworoads-trading.co.in" "ravi.parikh@tworoads.co.in" "hardik.dhakate@tworoads-trading.co.in" "subham.chawda@tworoads-trading.co.in";
#      exit;
    fi
}


fetch_stock_security () {

ftp -np $FTP_HOST <<SCRIPT
  user "ftpguest" "FTPGUEST" 
  cd Common
  cd NTNEAT
  binary 
  get security.gz
  quit 
SCRIPT
gunzip -df $FTP_DIR/security.gz
strings $FTP_DIR/security | grep "|EQ|" | awk -F"|" '{print $1,0,"STK",$2,0,0,"XX","STK",0,0}' > /spare/local/tradeinfo/NSE_Files/RefData/nse_eq_"$next_working_day"_contracts.txt
#zgrep "|EQ|" $FTP_DIR/security.gz | awk -F"|" '{print $1,0,"STK",$2,0,0,"XX","STK",0,0}' > /spare/local/tradeinfo/NSE_Files/RefData/nse_eq_"$next_working_day"_contracts.txt 
}

fetch_fo_ref_contracts () {

ftp -np $FTP_HOST <<SCRIPT
  user "FAOGUEST" "FAOGUEST" 
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

ftp -np $FTP_HOST <<SCRIPT 
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
#  wget --referer https://www1.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www1.nseindia.com/content/historical/DERIVATIVES/"$YYYY"/"$MSTR"/fo"$DD"$MSTR"$YYYY"bhav.csv.zip
wget --referer https://www.nseindia.com/  https://archives.nseindia.com/content/historical/DERIVATIVES/"$YYYY"/"$MSTR"/fo"$DD"$MSTR"$YYYY"bhav.csv.zip
  unzip fo"$DD"$MSTR"$YYYY"bhav.csv.zip
  rm fo"$DD"$MSTR"$YYYY"bhav.csv.zip
}


fetch_fo_bhavcopy () {

echo "FETCHING FILE FROM FTP";

ftp -np $FTP_HOST <<SCRIPT 
  user "FAOGUEST" "FAOGUEST" 
  cd faoftp 
  cd faocommon
  cd Bhavcopy 
  binary 
  get $DD$MM"0000.md"
  quit 
SCRIPT

if [ -e $DD$MM"0000".md ]; then
  echo "PRESENT" ;
  mv $DD$MM"0000".md $DD$MM"fo_0000".md
else
echo "Getting FO Bhavcopy from website.."
#Month str like FEB, MAR etc.
  MSTR=$(echo $(date -d $YYYYMMDD +%b) | awk '{print toupper($1)}') ;

#if file dowsnot exist on FTP, download from website
#  wget --referer https://www1.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www1.nseindia.com/content/historical/DERIVATIVES/"$YYYY"/"$MSTR"/fo"$DD"$MSTR"$YYYY"bhav.csv.zip ;

wget --referer https://www.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://archives.nseindia.com/content/historical/DERIVATIVES/"$YYYY"/"$MSTR"/fo"$DD"$MSTR"$YYYY"bhav.csv.zip

#Unzip and convert to FTP bhavcopy format
  unzip -o fo"$DD"$MSTR"$YYYY"bhav.csv.zip ;
  cat fo"$DD"$MSTR"$YYYY"bhav.csv | tail -n +2 | awk -F"," '{ OFS=","; gsub(/-/," ",$3); print "NORMAL",$1,$2,toupper($3),$4,$5,"-",$6,$7,$8,$9,$11,$12,$13,$14}' > $DD$MM"fo_0000".md ;
fi

chown dvctrader:infra $DD$MM"fo_0000".md
} 

fetch_cd_bhavcopy () {
ftp -np $FTP_HOST <<SCRIPT
  user "CDSGUEST" "CDSGUEST"  
  cd cdsftp
  cd cdscommon
  cd Bhavcopy 
  binary 
  get "FINAL_"$DD$MM"0000.md"
SCRIPT

if [ -e "FINAL_"$DD$MM"0000".md ];then 
  mv "FINAL_"$DD$MM"0000".md $DD$MM"cd_0000".md ;
else

echo "Getting CD Bhavcopy from website.."
#Get from website
#  wget --referer https://www1.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0"  https://www1.nseindia.com/archives/cd/bhav/CD_Bhavcopy"$DD$MM$YY".zip

#  wget --referer https://www1.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0"  https://www1.nseindia.com/archives/ird/sett/CDSett_prce_IRF_"$DD$MM$YYYY".csv

  wget --referer https://www.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0"  https://archives.nseindia.com/archives/cd/bhav/CD_Bhavcopy"$DD$MM$YY".zip

  wget --referer https://www.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0"  https://archives.nseindia.com/archives/ird/sett/CDSett_prce_IRF_"$DD$MM$YYYY".csv

#Unzip CD Bhavcopy, It gives two files for FO and OP
  unzip -o CD_Bhavcopy"$DD$MM$YY".zip

#convert the FO CD file
  cat CD_NSE_FO"$DD$MM$YY".csv | tail -n+2 | awk -F"," '{OFS=","; s1=substr($1,1,6); s2=substr($1,7,6); s3=substr($1,13,12); gsub(/-/," ",s3); print "NORMAL",s1,s2,s3," "," ",$2,$3,$4,$5,$6,$11,"-","-","-"}' > $DD$MM"cd_0000".md

#convert the OP CD file
  cat CD_NSE_OP"$DD$MM$YY".csv | tail -n+2 | awk -F"," '{OFS=","; s1=substr($1,1,6); s2=substr($1,7,6); s3=substr($1,13,11); s4=substr($1,24,2); s5=substr($1,26); gsub(/-/," ",s3); print "NORMAL",s1,s2,s3,s5,s4,$2,$3,$4,$5,$6,$11,"-","-","-"}' >> $DD$MM"cd_0000".md

#Convert Bond Futures file
  cat CDSett_prce_IRF_"$DD$MM$YYYY".csv | tail -n+2 | awk -F"," '{OFS=","; gsub(/-/," ",$4); print "NORMAL",$2,$3,$4," "," ","0",$5,$5,$5,$5,"-","-","-"}' >> $DD$MM"cd_0000".md

fi

chown dvctrader:infra $DD$MM"cd_0000".md
}

verify_given_file () {

  GIVEN_FILE=$1;
    [ -f $GIVEN_FILE -a -s $GIVEN_FILE -a -r $GIVEN_FILE ] || printf "Contract File not present for today" | /bin/mail -s "NSEDailyFilesGenerationFailure - $GIVEN_FILE" -r $HOSTNAME "raghunandan.sharma@tworoads-trading.co.in" "ravi.parikh@tworoads.co.in" "hardik.dhakate@tworoads-trading.co.in" "subham.chawda@tworoads-trading.co.in"
    [ -f $GIVEN_FILE -a -s $GIVEN_FILE -a -r $GIVEN_FILE ] || exit ;

}
verify_files () {

  FILENAME=$FTP_DIR/$1 ;  

  [ -f $FILENAME -a -s $FILENAME -a -r $FILENAME ] || printf "NSE_DAILY_FILE_GENERATION_FAILURE" | /bin/mail -s "NSEDailyFilesGenerationFailure - $FILENAME" -r $HOSTNAME "raghunandan.sharma@tworoads-trading.co.in" "ravi.parikh@tworoads.co.in" "hardik.dhakate@tworoads-trading.co.in" "subham.chawda@tworoads-trading.co.in"
  [ -f $FILENAME -a -s $FILENAME -a -r $FILENAME ] || exit ; 

}

get_expiry_date () {
  ONE=01;
  EXPIRY=$YYYYMMDD;
  for i in {1..7}; do dateStr=`date -d "$YYYY$MM$ONE + 1 month - $i day" +"%w %Y%m%d"`; if [ ${dateStr:0:1} -eq 4 ] ; then EXPIRY=${dateStr:2}; fi ; done
}

gen_physical_settlement_list() {
  #Contract File Is Present, We can generate physical settlement names
  ps_grp=`date -d  "${N_YYYY}-${N_MM}-01" "+%y%b" | awk '{print toupper($0)}'`
  grep "$ps_grp" contract | grep "|P|" | awk -F"|" '{print $4}' | sort | uniq > /spare/local/tradeinfo/NSE_Files/SecuritiesUnderPhysicalSettlement/fo_securities_under_physical_settlement.csv
  cp "/spare/local/tradeinfo/NSE_Files/SecuritiesUnderPhysicalSettlement/fo_securities_under_physical_settlement.csv" "/spare/local/tradeinfo/NSE_Files/SecuritiesUnderPhysicalSettlement/fo_securities_under_physical_settlement_${N_MM}${N_YY}.csv"
}

#Main 
if [ $# -ne 1 ] ; then 
  echo "Called As : " $* ; 
  print_usage_and_exit ;
fi 
echo "---------------------------------------------Starting Fetch NSE-------------------------------------"
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
   echo "NSE Holiday. Exiting...";
   exit;
fi 

FTP_DIR="/spare/local/files/NSEFTPFiles/$YYYY/$MM/$DD/"
rm -rf $FTP_DIR
mkdir -p $FTP_DIR
cd $FTP_DIR

#[ $is_holiday == "2" ] || printf "NSE_DAILY_FILE_GENERATION_FAILURE" | /bin/mail -s "NSEDailyFilesGenerationFailure - NSE_Holidays" -r "ravi.parikh@tworoads.co.in" "DVCTech@circulumvite.com" ; 
[ $is_holiday == "2" ] || exit ; 

next_working_day=`/home/pengine/prod/live_execs/update_date $YYYYMMDD N A`

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
while [[ $is_holiday = "1" ]] 
do
     next_working_day=`/home/pengine/prod/live_execs/update_date $next_working_day N A`
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
     prev_cd_working_day=`/home/pengine/prod/live_execs/update_date $prev_cd_working_day P A`
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
#verify_files "cd_spd_contract"

verify_files $DD$MM"fo_0000".md
echo "CHECKED";
verify_files $DD$MM"cd_0000".md

mkdir -p /spare/local/tradeinfo/NSE_Files/BhavCopy/cds/$MM$YY/
mkdir -p /spare/local/tradeinfo/NSE_Files/BhavCopy/fo/$MM$YY/ 

check_bhavcopy_expiry $FTP_DIR/$DD$MM"fo_0000".md;
#check_bhavcopy_expiry $FTP_DIR/$DD$MM"cd_0000".md;

 
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
N_YYYY=$YYYY
if [ $YYYYMMDD -ge $EXPIRY ] ; then
  NEXT_MONTH=`date -d "$YYYYMMDD + 1 month - 10 day" +"%m%Y"`
  N_MM=${NEXT_MONTH:0:2}
  N_YY=${NEXT_MONTH:4:2}
  N_YYYY=${NEXT_MONTH:2:4}
#  fetch_mkt_lots ;
  echo "NextMonth: $N_MM , $N_YY"
fi

gen_physical_settlement_list;

if [ $YYYYMMDD -eq $EXPIRY ] ; then

echo '/home/pengine/prod/live_scripts/NSE_File_Generator.pl /spare/local/tradeinfo/NSE_Files/BhavCopy/fo/$MM$YY/$DD$MM"fo_0000".md /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv"  /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_$N_MM$N_YY".csv" /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_working_day C /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt $next_working_day $EXPIRY' ; 

echo '/home/pengine/prod/live_scripts/NSE_File_Generator.pl /spare/local/tradeinfo/NSE_Files/BhavCopy/cds/$MM$YY/$DD$MM"cd_0000".md /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv"  /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_$N_MM$N_YY".csv" /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_working_day A /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt $next_working_day $EXPIRY';

/home/pengine/prod/live_scripts/NSE_File_Generator.pl /spare/local/tradeinfo/NSE_Files/BhavCopy/fo/$MM$YY/$DD$MM"fo_0000".md /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv"  /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_$N_MM$N_YY".csv" /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_working_day C /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt $next_working_day $EXPIRY

/home/pengine/prod/live_scripts/NSE_File_Generator.pl /spare/local/tradeinfo/NSE_Files/BhavCopy/cds/$MM$YY/$DD$MM"cd_0000".md /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv"  /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_$N_MM$N_YY".csv" /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_working_day A /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt $next_working_day $EXPIRY
else

echo '/home/pengine/prod/live_scripts/NSE_File_Generator.pl /spare/local/tradeinfo/NSE_Files/BhavCopy/fo/$MM$YY/$DD$MM"fo_0000".md /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv"  /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_$N_MM$N_YY".csv" /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_working_day C /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt $next_working_day';

echo '/home/pengine/prod/live_scripts/NSE_File_Generator.pl /spare/local/tradeinfo/NSE_Files/BhavCopy/cds/$MM$YY/$DD$MM"cd_0000".md /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv"  /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_$N_MM$N_YY".csv" /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_working_day A /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt $next_working_day';


/home/pengine/prod/live_scripts/NSE_File_Generator.pl /spare/local/tradeinfo/NSE_Files/BhavCopy/fo/$MM$YY/$DD$MM"fo_0000".md /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv"  /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_$N_MM$N_YY".csv" /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_working_day C /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt $next_working_day

/home/pengine/prod/live_scripts/NSE_File_Generator.pl /spare/local/tradeinfo/NSE_Files/BhavCopy/cds/$MM$YY/$DD$MM"cd_0000".md /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv"  /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_$N_MM$N_YY".csv" /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_working_day A /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt $next_working_day
fi

#Dated MktLots File
cp /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv" /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$next_working_day".csv";

cd /spare/local/tradeinfo/NSE_Files/SecuritiesUnderBan/
rm fo_secban.csv

echo "UPDATING BAN FILE"
NXT_DAY_YYYY=${next_working_day:0:4}
NXT_DAY_MM=${next_working_day:4:2}
NXT_DAY_DD=${next_working_day:6:2}
wget --referer https://www.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://archives.nseindia.com/archives/fo/sec_ban/fo_secban_${NXT_DAY_DD}${NXT_DAY_MM}${NXT_DAY_YYYY}.csv  #https://www1.nseindia.com/content/fo/fo_secban.csv
mv fo_secban_${NXT_DAY_DD}${NXT_DAY_MM}${NXT_DAY_YYYY}.csv fo_secban.csv
grep -v "Securities" fo_secban.csv | awk -F"," '{print $2}' > fo_secban_$next_working_day".csv"
#Fixing invisible character below
cat fo_secban_$next_working_day".csv" | sed 's/\r$//' > fo_secban_$next_working_day".csv_temp"
mv fo_secban_$next_working_day".csv_temp" fo_secban_$next_working_day".csv"

year=${next_working_day:0:4}
month=`date -d $next_working_day +"%b"`
next_day=${next_working_day:6:2}

if [ "`grep -i "$next_day.*$month.*$year" fo_secban.csv | wc -l `" -eq "0" ]; then
   error_msg="SecBan File for $next_working_day is not latest. Please download the file later"
   $send_slack_exec production-issues DATA "$error_msg"
   echo ""|mailx -s "Secunder ban file not updated update after some time" -r $HOSTNAME "raghunandan.sharma@tworoads-trading.co.in" "hardik.dhakate@tworoads-trading.co.in" "subham.chawda@tworoads-trading.co.in"
fi

echo "UPDATING SPAN FILE"
cd /spare/local/tradeinfo/NSE_Files/Margin_Files/Span_Files/
#wget --referer https://www1.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www1.nseindia.com/archives/nsccl/span/nsccl.$YYYYMMDD.s.zip
wget --referer https://www.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://archives.nseindia.com/archives/nsccl/span/nsccl.$YYYYMMDD.s.zip
unzip -o nsccl.$YYYYMMDD.s.zip
rm nsccl.$YYYYMMDD.s.zip
chmod 666 nsccl.$YYYYMMDD.s.spn
mv nsccl.$YYYYMMDD.s.spn nsccl.$YYYYMMDD.s_1.spn
prev_day=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P A 1`
verify_given_file /spare/local/tradeinfo/NSE_Files/Margin_Files/Span_Files/nsccl.$YYYYMMDD.s_1.spn

echo "Updating EXPOSURE FILE"
cd /spare/local/tradeinfo/NSE_Files/Margin_Files/Exposure_Files
#wget --referer https://www1.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www1.nseindia.com/archives/exp_lim/ael_$DD$MM$YYYY.csv
wget --referer https://www.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://archives.nseindia.com/archives/exp_lim/ael_$DD$MM$YYYY.csv

cat ael_$DD$MM$YYYY.csv | awk -F "," '{print $1,$2,$5}' > exposure_margin_rates.$YYYYMMDD
rm ael_$DD$MM$YYYY.csv

echo "UPDATING SECURITY MARGIN EQ"
>"/spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles/security_margin_"$next_working_day".txt"
month_name=`date -d $YYYYMMDD '+%b' | awk '{print toupper($0)}'`
#wget --referer https://www1.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www1.nseindia.com/archives/nsccl/var/C_VAR1_"$DD$MM$YYYY"_6.DAT
#wget --referer https://www1.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www1.nseindia.com/content/historical/EQUITIES/"$YYYY"/"$month_name"/cm"$DD""$month_name""$YYYY"bhav.csv.zip

wget --referer https://www.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://archives.nseindia.com/archives/nsccl/var/C_VAR1_"$DD$MM$YYYY"_6.DAT
wget --referer https://www.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://archives.nseindia.com/content/historical/EQUITIES/"$YYYY"/"$month_name"/cm"$DD""$month_name""$YYYY"bhav.csv.zip

verify_given_file /spare/local/tradeinfo/NSE_Files/Margin_Files/Exposure_Files/C_VAR1_"$DD$MM$YYYY"_6.DAT
if [ ! -f cm"$DD""$month_name""$YYYY"bhav.csv.zip ]; then
  error_msg="Unable to download CM Bhavcopy file for $YYYYMMDD. Please download the file manually"
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

rm cm_var_"$DD$MM$YYYY" cm_var_"$DD$MM$YYYY"_copy cm_loss_rate_"$DD$MM$YYYY" cm_closing_price_"$DD$MM$YYYY"  C_VAR1_"$DD$MM$YYYY"_6.DAT

echo "UPDATING DB"
#Update midterm_db with Lotsizes for next trading day
if test `find "/spare/local/tradeinfo/NSE_Files/midterm_db" -mmin +600`
then
  cp /spare/local/tradeinfo/NSE_Files/midterm_db /spare/local/tradeinfo/NSE_Files/midterm_db.bkp
fi
cp /spare/local/tradeinfo/NSE_Files/midterm_db /home/dvctrader/trash/midterm_db
/apps/anaconda/anaconda3/bin/python /home/pengine/prod/live_scripts/Update_MidTermDB_Lotsizes.py  --date $next_working_day --db_path /home/dvctrader/trash/midterm_db
mv /home/dvctrader/trash/midterm_db /spare/local/tradeinfo/NSE_Files/midterm_db
chown dvctrader:infra /spare/local/tradeinfo/NSE_Files/midterm_db

echo "VERIFY FILES"
cd /spare/local/tradeinfo/NSE_Files/Margin_Files/Span_Files/ ;
#find -type f -mtime +2 -exec rm -f {} \; 

verify_given_file /spare/local/tradeinfo/NSE_Files/Margin_Files/Exposure_Files/exposure_margin_rates.$YYYYMMDD
verify_given_file /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_working_day
verify_given_file /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt

#Update Fo Next trading per symbol unit margins
declare -A hashmap
margin_exec_="/home/pengine/prod/live_execs/calc_nse_span_multi_products"
exch_sym_exec_="/home/pengine/prod/live_execs/get_exchange_symbol_file"
symbols="/home/pengine/prod/live_configs/nse_midterm_shortcodes"
symbols_1="/home/pengine/prod/live_configs/nse_midterm_shortcodes_1"
#symbols="/home/pengine/prod/live_configs/nse_midterm_shortcodes_temp"
ds_sym_file="/spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt"
temp_shc_file_="/home/dvctrader/trash/temp_shcs"
temp_shc_file_opt="/home/dvctrader/trash/temp_shcs_opt"
contract_file_="/spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts."$next_working_day
exchnge_symbol="/tmp/exchange_symbol_generated"
security_margin_file="/spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles/security_margin_$next_working_day.txt"
echo "generate the symbol shortcode"
echo "$exch_sym_exec_ $symbols $next_working_day"
$exch_sym_exec_ $symbols $next_working_day | grep -v "INVALID" >$exchnge_symbol
echo "$exch_sym_exec_ $symbols_1 $next_working_day"
$exch_sym_exec_ $symbols_1 $next_working_day | grep -v "INVALID" >>$exchnge_symbol

>$temp_shc_file_opt
echo "Computing Option For SM"
#For options
for i in `cat $symbols`; do
  if [[ "`echo $i | cut -d'_' -f3 | grep FUT`" == "" ]]; then
       stock_=`echo $i | cut -d'_' -f2`
       ex_sym=`grep $i $exchnge_symbol | head -n 1 | cut -d' ' -f2`
       [ -z $ex_sym ]  && continue;
       ds_sym=`grep $ex_sym $ds_sym_file | awk 'BEGIN{FS=" "}{print $2}'`
       echo -e "$ds_sym\t-1" >> $temp_shc_file_opt
       hashmap[$ds_sym]=$i
  fi
done
# second file 
for i in `cat $symbols_1`; do
  if [[ "`echo $i | cut -d'_' -f3 | grep FUT`" == "" ]]; then
       stock_=`echo $i | cut -d'_' -f2`
       ex_sym=`grep $i $exchnge_symbol | head -n 1 | cut -d' ' -f2`
       [ -z $ex_sym ]  && continue;
       ds_sym=`grep $ex_sym $ds_sym_file | awk 'BEGIN{FS=" "}{print $2}'`
       echo -e "$ds_sym\t-1" >> $temp_shc_file_opt
       hashmap[$ds_sym]=$i
      fi
done

tmp_option="/tmp/file_output_options"
>/tmp/_options_sec_margin
$margin_exec_ $YYYYMMDD $temp_shc_file_opt >$tmp_option
while IFS=" " read -r val1 val2 ;do
    echo "${hashmap[$val1]} $val2" >> /tmp/_options_sec_margin
done < $tmp_option

awk '{if($2 > 0)print $0}' /tmp/_options_sec_margin >> $security_margin_file

#For all futures from Contract File
echo "Computing Futures For SM"
declare -A expmap
>$temp_shc_file_
expiries=(`cat $contract_file_ | grep "IDXFUT" | awk 'BEGIN{FS="\t"}{print $6}' | sort -u`)
for i in `seq 0 $((${#expiries[@]}-1))`; do  expmap[${expiries[i]}]=$i ; done
for i in `cat $contract_file_ | egrep "IDXFUT|STKFUT" | awk 'BEGIN{FS="\t"; OFS=","}{print $2,$6}'`; do
    stk=`echo $i | cut -d',' -f1`
    exp=`echo $i | cut -d',' -f2`
    ds_sym="NSE_"$stk"_FUT_"$exp
    shc="NSE_"$stk"_FUT"${expmap[$exp]}
    echo -e "$ds_sym\t-1" >> $temp_shc_file_
    hashmap[$ds_sym]=$shc
done
tmp_fut="/tmp/file_output_futures"
$margin_exec_ $YYYYMMDD $temp_shc_file_ >$tmp_fut
while IFS=" " read -r val1 val2 ;do
    echo "${hashmap[$val1]} $val2" >> $security_margin_file
done < $tmp_fut
echo "SM File generated"
echo "add_security_missing_margin.sh $YYYYMMDD"
/home/pengine/prod/live_scripts/add_security_missing_margin.sh $YYYYMMDD
echo "NIFTY AND BANK ADDED"

/home/pengine/prod/live_scripts/check_daily_files.sh >/tmp/verify_out 2>/tmp/verify_err
find /spare/local/tradeinfo/NSE_Files/DataExchFile -type f -mtime +7 -exec rm -f {} \;

#echo "Removing Undwanted Symbols"
#/home/dvctrader/important/handling_Updates/exclude_from_sec_margin.sh $YYYYMMDD
echo "NSEFILESUPDATED"

#/home/dvctrader/important/onexpiry/update_contract_for_july_midterm.sh $YYYYMMDD

rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.62:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.61:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.63:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.64:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.65:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.69:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.81:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.82:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.83:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.84:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.66:/spare/local/tradeinfo --delete-after
#req sync for ors reply report script
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.5.67:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.5.42:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.71:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.72:/spare/local/tradeinfo --delete-after

scp /spare/local/files/stocks.db dvcinfra@10.23.227.61:/spare/local/files/
scp /spare/local/files/stocks.db dvcinfra@10.23.227.62:/spare/local/files/
scp /spare/local/files/stocks.db dvcinfra@10.23.227.63:/spare/local/files/
scp /spare/local/files/stocks.db dvcinfra@10.23.227.64:/spare/local/files/
scp /spare/local/files/stocks.db dvcinfra@10.23.227.65:/spare/local/files/
scp /spare/local/files/stocks.db dvcinfra@10.23.227.69:/spare/local/files/
scp /spare/local/files/stocks.db dvcinfra@10.23.227.81:/spare/local/files/
scp /spare/local/files/stocks.db dvcinfra@10.23.227.82:/spare/local/files/
scp /spare/local/files/stocks.db dvcinfra@10.23.227.83:/spare/local/files/
scp /spare/local/files/stocks.db dvcinfra@10.23.227.84:/spare/local/files/
scp /spare/local/files/stocks.db dvcinfra@10.23.227.66:/spare/local/files/
scp /spare/local/files/stocks.db dvcinfra@10.23.227.71:/spare/local/files/
scp /spare/local/files/stocks.db dvcinfra@10.23.227.72:/spare/local/files/

#rsync -avz --progress /spare/local/tradeinfo/NSE_Files 3.89.148.73:/spare/local/tradeinfo
rsync -avz --progress /spare/local/tradeinfo/NSE_Files 54.90.155.232:/spare/local/tradeinfo

rsync -ravz /spare/local/files/NSEFTPFiles 10.23.227.61:/spare/local/files --delete-after
rsync -ravz /spare/local/files/NSEFTPFiles 10.23.227.83:/spare/local/files --delete-after
rsync -ravz /spare/local/files/NSEFTPFiles 10.23.227.84:/spare/local/files --delete-after
rsync -ravz /spare/local/files/NSEFTPFiles 10.23.227.63:/spare/local/files --delete-after
rsync -ravz /spare/local/files/NSEFTPFiles 10.23.227.64:/spare/local/files --delete-after
rsync -ravz /spare/local/files/NSEFTPFiles 10.23.227.65:/spare/local/files --delete-after
rsync -ravz /spare/local/files/NSEFTPFiles 10.23.227.69:/spare/local/files --delete-after
rsync -ravz /spare/local/files/NSEFTPFiles 10.23.227.81:/spare/local/files --delete-after
rsync -ravz /spare/local/files/NSEFTPFiles 10.23.227.82:/spare/local/files --delete-after
rsync -ravz /spare/local/files/NSEFTPFiles 10.23.227.66:/spare/local/files --delete-after
rsync -ravz /spare/local/files/NSEFTPFiles 10.23.227.71:/spare/local/files --delete-after
rsync -ravz /spare/local/files/NSEFTPFiles 10.23.227.72:/spare/local/files --delete-after
#rsync -avz /spare/local/files/NSEFTPFiles 3.89.148.73:/spare/local/files
rsync -avz /spare/local/files/NSEFTPFiles 54.90.155.232:/spare/local/files
