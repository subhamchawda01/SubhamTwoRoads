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
EXPS=($(cat /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_date | grep USDINR | grep FUT | awk 'BEGIN{FS="\t"}{print $6}' | sort))
counter=0
while [ ${#EXPS[@]} -eq 0 ] && [ $counter -lt 7 ]
do
        next_date=$(/home/dvctrader/midterm/basetrade_install/bin/update_date $next_date N W 1)
        EXPS=($(cat /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_date | grep USDINR | grep FUT | awk 'BEGIN{FS="\t"}{print $6}' | sort))
	counter=$(( $counter + 1 ))
done
if [ ${#EXPS[@]} -eq 0 ]
then 
	exit 1
fi
echo -e $date_"\t"${EXPS[0]}"\t"${EXPS[1]}"\t"${EXPS[2]}"\t"${EXPS[3]}"\t"${EXPS[4]}"\t"${EXPS[5]} >> /home/dvctrader/midterm/Data_Generator/expiries_cd.txt

#Generate the futures data filenames
rm FILENAMES
rm Generated_Data/*
touch /media/shared/ephemeral*/s3_cache/NAS1/data/NSELoggedData/NSE/$year/$month/$day/*_FUT_*
find /media/shared/ephemeral*/s3_cache/NAS1/data/NSELoggedData/NSE/$year/$month/$day/*USDINR_FUT_* -type f >> FILENAMES
find /media/shared/ephemeral*/s3_cache/NAS1/data/NSELoggedData/NSE/$year/$month/$day/*JPYINR_FUT_* -type f >> FILENAMES
find /media/shared/ephemeral*/s3_cache/NAS1/data/NSELoggedData/NSE/$year/$month/$day/*EURINR_FUT_* -type f >> FILENAMES
find /media/shared/ephemeral*/s3_cache/NAS1/data/NSELoggedData/NSE/$year/$month/$day/*GBPINR_FUT_* -type f >> FILENAMES
if [[ ! -s FILENAMES ]];
then
	exit 2
fi

#Generate the futures data
cd ..
/home/dvctrader/midterm/Data_Generator/historical_bar_generator_cd

#Sort the generated futures data
cd Data_Generator/Generated_Data
for i in *; do sort -n -k 1 $i >> /home/dvctrader/midterm/Data_Generator/Sorted_CD_Data/$i; done

