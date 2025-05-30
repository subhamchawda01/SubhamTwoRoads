#!/bin/bash

FTP_HOST='ftp.connect2nse.com'

get_expiry_date () {
    ONE=01;
    EXPIRY=$YYYYMMDD;
    for i in {1..7}; do dateStr=`date -d "$YYYY$MM$ONE + 1 month - $i day" +"%w %Y%m%d"`; if [ ${dateStr:0:1} -eq 4 ] ; then EXPIRY=${dateStr:2}; fi ; done
}

fetch_cd_bhavcopy () {
ftp -np $FTP_HOST <<SCRIPT
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
  wget --referer https://www.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0"  https://www1.nseindia.com/archives/cd/bhav/CD_Bhavcopy"$DD$MM$YY".zip
  wget --referer https://www.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0"  https://www1.nseindia.com/archives/ird/sett/CDSett_prce_IRF_"$DD$MM$YYYY".csv
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
  wget --referer https://www.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" https://www1.nseindia.com/content/historical/DERIVATIVES/"$YYYY"/"$MSTR"/fo"$DD"$MSTR"$YYYY"bhav.csv.zip ;
#Unzip and convert to FTP bhavcopy format
  unzip -o fo"$DD"$MSTR"$YYYY"bhav.csv.zip ;
  cat fo"$DD"$MSTR"$YYYY"bhav.csv | tail -n +2 | awk -F"," '{ OFS=","; gsub(/-/," ",$3); print "NORMAL",$1,$2,toupper($3),$4,$5,"-",$6,$7,$8,$9,$11,$12,$13,$14}' > $DD$MM"fo_0000".md ;
fi
chown dvctrader:infra $DD$MM"fo_0000".md
}

GetPreviousWorkingDay() {
  YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P W`
  is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  while [ $is_holiday_ = "1" ];
  do
    YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P W`
    is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  done
}

HHMM=`date +"%H%M"`
YYYYMMDD=`date +"%Y%m%d"`
if [ ${HHMM} -lt 1000 ];
then
  GetPreviousWorkingDay;
fi
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi
DD=${YYYYMMDD:6:2}
MM=${YYYYMMDD:4:2}
YYYY=${YYYYMMDD:0:4};
YY=${YYYYMMDD:2:2};

next_working_day=`/home/pengine/prod/live_execs/update_date $YYYY$MM$DD N W`
isholiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
while [ $is_holiday = "1" ] 
do
     next_working_day=`/home/pengine/prod/live_execs/update_date $next_working_day N W`
     is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
done
echo $next_working_day

#update YYYYMMDD to previous working day
GetPreviousWorkingDay;
FTP_DIR="/spare/local/files/NSEFTPFiles";
mkdir -p ${FTP_DIR}"/"${YYYY}"/"${MM}"/"${DD}"/";
cd ${FTP_DIR}"/"${YYYY}"/"${MM}"/"${DD}"/";
[[ $1 == "FORCE" ]] && `rm -rf ${FTP_DIR}"/"${YYYY}"/"${MM}"/"${DD}/$DD$MM"fo_0000".md ${FTP_DIR}"/"${YYYY}"/"${MM}"/"${DD}"/"$DD$MM"cd_0000.md"`
#simply exit if the file is already updated
[[  -f ${FTP_DIR}"/"${YYYY}"/"${MM}"/"${DD}"/"$DD$MM"fo_0000.md" ]] && [[ -f ${FTP_DIR}"/"${YYYY}"/"${MM}"/"${DD}"/"$DD$MM"cd_0000.md" ]] && { echo "File exist"; exit;}
[[  -f ${FTP_DIR}"/"${YYYY}"/"${MM}"/"${DD}"/"$DD$MM"fo_0000.md" ]] || fetch_fo_bhavcopy;
[[  -f ${FTP_DIR}"/"${YYYY}"/"${MM}"/"${DD}"/"$DD$MM"cd_0000.md" ]] || fetch_cd_bhavcopy;

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
   mv $FTP_DIR"/"${YYYY}"/"${MM}"/"${DD}"/"$prev_DD$prev_MM"cd_0000.md" $FTP_DIR"/"${YYYY}"/"${MM}"/"${DD}"/"$DD$MM"cd_0000.md"
fi

mkdir -p /spare/local/tradeinfo/NSE_Files/BhavCopy/fo/$MM$YY/
mkdir -p /spare/local/tradeinfo/NSE_Files/BhavCopy/cds/$MM$YY/
cp $FTP_DIR"/"${YYYY}"/"${MM}"/"${DD}"/"$DD$MM"fo_0000".md /spare/local/tradeinfo/NSE_Files/BhavCopy/fo/$MM$YY/
cp $FTP_DIR"/"${YYYY}"/"${MM}"/"${DD}"/"$DD$MM"cd_0000".md /spare/local/tradeinfo/NSE_Files/BhavCopy/cds/$MM$YY/

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
  echo "NextMonth: $N_MM , $N_YY"
fi

if [ $YYYYMMDD -eq $EXPIRY ] ; then

echo '/home/pengine/prod/live_scripts/NSE_File_Generator.pl /spare/local/tradeinfo/NSE_Files/BhavCopy/fo/$MM$YY/$DD$MM"fo_0000".md /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv"  /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_$N_MM$N_YY".csv" /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_working_day C /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt $EXPIRY' ;

echo '/home/pengine/prod/live_scripts/NSE_File_Generator.pl /spare/local/tradeinfo/NSE_Files/BhavCopy/cds/$MM$YY/$DD$MM"cd_0000".md /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv"  /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_$N_MM$N_YY".csv" /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_working_day A /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt $EXPIRY';

/home/pengine/prod/live_scripts/NSE_File_Generator.pl /spare/local/tradeinfo/NSE_Files/BhavCopy/fo/$MM$YY/$DD$MM"fo_0000".md /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv"  /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_$N_MM$N_YY".csv" /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_working_day C /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt $EXPIRY

/home/pengine/prod/live_scripts/NSE_File_Generator.pl /spare/local/tradeinfo/NSE_Files/BhavCopy/cds/$MM$YY/$DD$MM"cd_0000".md /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv"  /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_$N_MM$N_YY".csv" /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_working_day A /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt $EXPIRY
else

echo '/home/pengine/prod/live_scripts/NSE_File_Generator.pl /spare/local/tradeinfo/NSE_Files/BhavCopy/fo/$MM$YY/$DD$MM"fo_0000".md /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv"  /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_$N_MM$N_YY".csv" /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_working_day C /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt';

echo '/home/pengine/prod/live_scripts/NSE_File_Generator.pl /spare/local/tradeinfo/NSE_Files/BhavCopy/cds/$MM$YY/$DD$MM"cd_0000".md /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv"  /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_$N_MM$N_YY".csv" /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_working_day A /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt';

/home/pengine/prod/live_scripts/NSE_File_Generator.pl /spare/local/tradeinfo/NSE_Files/BhavCopy/fo/$MM$YY/$DD$MM"fo_0000".md /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv"  /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_$N_MM$N_YY".csv" /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_working_day C /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt

/home/pengine/prod/live_scripts/NSE_File_Generator.pl /spare/local/tradeinfo/NSE_Files/BhavCopy/cds/$MM$YY/$DD$MM"cd_0000".md /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv"  /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_$N_MM$N_YY".csv" /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_working_day A /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt
fi

#Update midterm_db with Lotsizes for next trading day
cp /spare/local/tradeinfo/NSE_Files/midterm_db /spare/local/tradeinfo/NSE_Files/midterm_db.bkp
cp /spare/local/tradeinfo/NSE_Files/midterm_db /home/dvctrader/trash/midterm_db
/apps/anaconda/anaconda3/bin/python /home/dvctrader/infracore/scripts/Update_MidTermDB_Lotsizes.py $next_working_day /home/dvctrader/trash/midterm_db
mv /home/dvctrader/trash/midterm_db /spare/local/tradeinfo/NSE_Files/midterm_db
chown dvctrader:infra /spare/local/tradeinfo/NSE_Files/midterm_db

echo "" | mailx -s "MD BHAVCOPY FILE UPDATED" -r $HOSTNAME raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in
