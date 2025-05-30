#!/bin/bash

USAGE="$0 DATE OLDPRODUCT(TATAGLOBAL) NEWPRODUCT(TATACONSUM)";

tradeinfo_dir='/spare/local/tradeinfo/NSE_Files/'
GetNearestExpiry() {
    contract_file=${tradeinfo_dir}'/ContractFiles/nse_contracts.'${date_}
    expiry=`cat ${contract_file} | grep IDXFUT | grep BANKNIFTY | awk -v date=${date_} '{if($NF>=date)print $NF'} | sort | uniq | head -n1`
}

if [ $# -ne 3 ] ;
then
    echo $USAGE
    exit;
fi

date_=$1
YYYYMMDD=$1 ;
DD=${YYYYMMDD:6:2}
MM=${YYYYMMDD:4:2}
YY=${YYYYMMDD:2:2}
YYYY=${YYYYMMDD:0:4}
month_name=`date -d $YYYYMMDD '+%b' | awk '{print toupper($0)}'`

old_sym=$2
new_sym=$3

next_working_day=`/home/pengine/prod/live_execs/update_date $date_ N A`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
while [ $is_holiday = "1" ] 
do
     next_working_day=`/home/pengine/prod/live_execs/update_date $next_working_day N A`
     is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
done
echo  "NextDay: $next_working_day";

echo "Taking BackUp"
backup_dir="/home/dvctrader/important/backup_fetch/"
cp /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_working_day $backup_dir/nse_contracts.${next_working_day}_bkp
cp /spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles/security_margin_${next_working_day}.txt $backup_dir/security_margin_${next_working_day}.txt_bkp
cp /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt $backup_dir/datasource_exchsymbol.txt_${date_}_bkp
cp /home/spare/local/tradeinfo/NSE_Files/Margin_Files/Exposure_Files/cm"$DD""$month_name""$YYYY"bhav.csv /home/spare/local/tradeinfo/NSE_Files/Margin_Files/Exposure_Files/cm"$DD""$month_name""$YYYY"bhav.csv_bkp
echo "contract file update"
sed -i "s/$old_sym/$new_sym/g" /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_working_day
echo "Updating Secuirty margin File"
sed -i "s/$old_sym/$new_sym/g" /spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles/security_margin_${next_working_day}.txt
echo "Update bhavcopy for CM"
sed -i "s/$old_sym/$new_sym/g" /home/spare/local/tradeinfo/NSE_Files/Margin_Files/Exposure_Files/cm"$DD""$month_name""$YYYY"bhav.csv
echo "db update"
/apps/anaconda/anaconda3/bin/python /home/pengine/prod/live_scripts/Update_MidTermDB_Lotsizes.py  --date $next_working_day --db_path /spare/local/tradeinfo/NSE_Files/midterm_db
chown dvctrader:infra /spare/local/tradeinfo/NSE_Files/midterm_db

echo "remove any entry of the new Symbol in datasource"
tmp_data="/tmp/datasource_exch.txt"
data_source="/spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt"
grep -v $new_sym $data_source > $tmp_data
mv  $tmp_data $data_source
echo "Updating datasource exchange symbol"
last_num=`awk -F"NSE" '{print $2}' $data_source | tail -1`
last_num=$((last_num+1))
GetNearestExpiry;
tmp_file="/tmp/file_data_symbol.txt"
grep $old_sym $data_source | awk '{print $2}' | awk -v strt=$last_num -v expy=$expiry -F'_' '{if (expy<=$4){ printf "NSE%d\t%s\n",strt,$0;strt++}}' | sed "s/$old_sym/$new_sym/g" >$tmp_file
cat $tmp_file >> $data_source

echo "Updating LotSize and Sos file"
sed -i "s/$old_sym/$new_sym/g" /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$next_working_day".csv"
sed -i "s/$old_sym/$new_sym/g" /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_${MM}${YY}.csv
cp /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$next_working_day".csv" /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_${MM}${YY}.csv
chgrp infra /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt
chgrp infra /spare/local/tradeinfo/NSE_Files/midterm_db
chgrp infra /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_${MM}${YY}.csv
chgrp infra /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_${MM}${YY}.csv
