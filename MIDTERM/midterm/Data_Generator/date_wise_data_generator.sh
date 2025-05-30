#!/bin/bash
#args=("$@")

#Obtain the date
if [ $# -eq 0 ];
then 
	date_="$(date +'%Y%m%d')"
else
	date_=$1
fi
year=${date_:0:4}
month=${date_:4:2}
day=${date_:6:2}

cd /home/dvctrader/midterm/Data_Generator/

#Update the expiry file
next_date=$(/home/dvctrader/midterm/basetrade_install/bin/update_date $date_ N W 1)
data=($(cat /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_date | grep NIFTY | grep -v BANK ) )
EXPS=($(echo ${data[*]} | awk '{print $6" "$12" "$18} '))
IFS=$'\n'
EXPS=($(sort <<<"${EXPS[*]}"))
unset IFS
echo -e $date_"\t"${EXPS[0]}"\t"${EXPS[1]}"\t"${EXPS[2]} >> /home/dvctrader/midterm/Data_Generator/expiries.txt

#Generate the futures data filenames
rm FILENAMES
rm Generated_Data/*
find /media/shared/ephemeral*/s3_cache/NAS1/data/NSELoggedData/NSE/$year/$month/$day/*_FUT_* -type f >> FILENAMES

#Generate the futures data
cd ..
./med_exec_install/bin/historical_bar_generator

#Sort the generated futures data
cd Data_Generator/Generated_Data
for i in *; do sort -n -k 1 $i >> /home/dvctrader/midterm/Data_Generator/Sorted_Data/$i; done
