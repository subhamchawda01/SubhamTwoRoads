#!/bin/bash

print_usage_and_exit () {
    echo "USAGE: $0 YYYYMMDD" ;  
    echo "Please change 2 variables : OUTPUT_FILE_DIR and FILE_GENERATOR_SCRIPT_PATH in the scrpit" ;
    exit ; 
} 

get_expiry_date () {
  ONE=01;
  EXPIRY=$YYYYMMDD;
  for i in {1..7}; do dateStr=`date -d "$YYYY$MM$ONE + 1 month - $i day" +"%w %Y%m%d"`; if [ ${dateStr:0:1} -eq 4 ] ; then EXPIRY=${dateStr:2}; fi ; done
}

#Please change this for output directory
OUTPUT_FILE_DIR="/home/dvctrader/NSE_NEW_FILES/"

#Please change the script path you want to use
FILE_GENERATOR_SCRIPT_PATH="$HOME/NSE_File_Generator.pl"

#Main 
if [ $# -ne 1 ] ; then 
  print_usage_and_exit ;
fi 

YYYYMMDD=$1 ; 
DD=${YYYYMMDD:6:2} 
MM=${YYYYMMDD:4:2} 
YY=${YYYYMMDD:2:2}
YYYY=${YYYYMMDD:0:4}

is_holiday=`/home/dvcinfra/infracore_install/bin/holiday_manager EXCHANGE NSE $YYYYMMDD T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday : $YYYYMMDD. Exiting...";
   exit;
fi 

next_working_day=`$HOME/infracore_install/bin/update_date $YYYYMMDD N W`

is_holiday=`/home/dvcinfra/infracore_install/bin/holiday_manager EXCHANGE NSE $next_working_day T`
while [ $is_holiday = "1" ] 
do
     next_working_day=`$HOME/infracore_install/bin/update_date $next_working_day N W`
     is_holiday=`/home/dvcinfra/infracore_install/bin/holiday_manager EXCHANGE NSE $next_working_day T`
done

# If today is a CD holiday, we look for the last working day for CD and use the file of that date for creating nse_contract file
prev_cd_working_day=$YYYYMMDD
is_cd_holiday=`/home/dvcinfra/infracore_install/bin/holiday_manager EXCHANGE NSE_CD $prev_cd_working_day T`
while [ $is_cd_holiday = "1" ] 
do
     prev_cd_working_day=`$HOME/infracore_install/bin/update_date $prev_cd_working_day P W`
     is_cd_holiday=`/home/dvcinfra/infracore_install/bin/holiday_manager EXCHANGE NSE_CD $prev_cd_working_day T`
done
CD_DD=${prev_cd_working_day:6:2} 
CD_MM=${prev_cd_working_day:4:2} 
CD_YY=${prev_cd_working_day:2:2}


get_expiry_date;

#echo "Expiry : $EXPIRY";
N_MM=$MM
N_YY=$YY
# If Today's date is after this month's expiry, then we need to use next month's mkt_lots and lot_size file
if [ $YYYYMMDD -ge $EXPIRY ] ; then
  NEXT_MONTH=`date -d "$YYYYMMDD + 1 month - 1 day" +"%m%y"`
  N_MM=${NEXT_MONTH:0:2}
  N_YY=${NEXT_MONTH:2:2}

  #echo "NextMonth: $N_MM , $N_YY"
fi

echo "Generating for $next_working_day . UsingCDDate: $prev_cd_working_day . MktSoSDate: $N_MM$N_YY"

if [ ! -e /spare/local/tradeinfo/NSE_Files/BhavCopy/fo/$MM$YY/$DD$MM"fo_0000".md ] ; then 
	echo "FO BhavCopy: /spare/local/tradeinfo/NSE_Files/BhavCopy/fo/$MM$YY/$DD$MM"fo_0000".md does not exist. Exiting"
	exit
fi

if [ ! -e /spare/local/tradeinfo/NSE_Files/BhavCopy/cds/$CD_MM$CD_YY/$CD_DD$CD_MM"cd_0000".md ] ; then 
	echo "CD BhavCopy: /spare/local/tradeinfo/NSE_Files/BhavCopy/cds/$CD_MM$CD_YY/$CD_DD$CD_MM"cd_0000".md does not exist. Exiting"
	exit
fi

if [ ! -e /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv" ] ; then 
	echo "MktLots: /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv" does not exist. Exiting"
	exit
fi

if [ ! -e /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_$N_MM$N_YY".csv" ] ; then 
	echo "SOSFile: /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_$N_MM$N_YY".csv" does not exist. Exiting"
	exit
fi


if [ $YYYYMMDD -eq $EXPIRY ] ; then
$FILE_GENERATOR_SCRIPT_PATH /spare/local/tradeinfo/NSE_Files/BhavCopy/fo/$MM$YY/$DD$MM"fo_0000".md /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv"  /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_$N_MM$N_YY".csv" $OUTPUT_FILE_DIR/nse_contracts.$next_working_day C $OUTPUT_FILE_DIR/datasource_exchsymbol.txt $EXPIRY

$FILE_GENERATOR_SCRIPT_PATH /spare/local/tradeinfo/NSE_Files/BhavCopy/cds/$CD_MM$CD_YY/$CD_DD$CD_MM"cd_0000".md /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv"  /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_$N_MM$N_YY".csv" $OUTPUT_FILE_DIR/nse_contracts.$next_working_day A $OUTPUT_FILE_DIR/datasource_exchsymbol.txt $EXPIRY
else
$FILE_GENERATOR_SCRIPT_PATH /spare/local/tradeinfo/NSE_Files/BhavCopy/fo/$MM$YY/$DD$MM"fo_0000".md /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv"  /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_$N_MM$N_YY".csv" $OUTPUT_FILE_DIR/nse_contracts.$next_working_day C $OUTPUT_FILE_DIR/datasource_exchsymbol.txt

$FILE_GENERATOR_SCRIPT_PATH /spare/local/tradeinfo/NSE_Files/BhavCopy/cds/$CD_MM$CD_YY/$CD_DD$CD_MM"cd_0000".md /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$N_MM$N_YY".csv"  /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_$N_MM$N_YY".csv" $OUTPUT_FILE_DIR/nse_contracts.$next_working_day A $OUTPUT_FILE_DIR/datasource_exchsymbol.txt 
fi


#cp /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt ~/NSE_NEW_FILES/datasource_exchsymbol.txt ; DATE=20141014; while [ $DATE -lt 20150401 ] ; do echo $DATE >> ~/out_regen ; ~/regenerate_nse_contract_files.sh $DATE >>~/out_regen 2>&1 ; DATE=`/home/dvctrader/infracore_install/bin/update_date $DATE N W` ; done


