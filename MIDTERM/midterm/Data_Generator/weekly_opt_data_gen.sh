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

#Obtain relevant expiries
next_date=$( /home/dvctrader/cvquant_install/dvccode/bin/update_date $date_ N W 1)
data=($(cat /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_date | grep NIFTY | grep -v BANK ) )
counter=0
while [ ${#data[@]} -eq 0 ] && [ $counter -lt 7 ]
do
        next_date=$( /home/dvctrader/cvquant_install/dvccode/bin/update_date $next_date N W 1)
        data=($(cat /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_date | grep NIFTY | grep -v BANK ) )
	counter=$(( $counter + 1 ))
done
EXPS=($(echo ${data[*]} | awk '{print $6" "$12" "$18} '))
if [ ${#EXPS[@]} -eq 0 ]
then
	exit 1
fi
IFS=$'\n'
EXPS=($(sort <<<"${EXPS[*]}"))
unset IFS
#echo -e $date_"\t"${EXPS[0]}"\t"${EXPS[1]}"\t"${EXPS[2]} >> /home/dvctrader/midterm/Data_Generator/expiries_weekly.txt

data=($(cat /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_date | grep BANKNIFTY | grep IDXOPT ) )

WEEKLY_EXPS=($(echo ${data[*]} | awk '{print $6" "$16" "$26" "$36" "$46" "$56" "$66" "$76" "$86" "$96" "$106} '))
#for i in "${EXPS[@]}"; 
#do
#	WEEKLY_EXPS=(${WEEKLY_EXPS[@]//*$i*})
#done

IFS=$'\n'
WEEKLY_EXPS=($(sort <<<"${WEEKLY_EXPS[*]}"))
unset IFS
check=`([ ${WEEKLY_EXPS[10]+abc} ] && echo "exists")`
if [ $check=="" ];
then
	WEEKLY_EXPS[10]=-1
fi
check=`([ ${WEEKLY_EXPS[9]+abc} ] && echo "exists")`
if [ $check=="" ];
then
        WEEKLY_EXPS[9]=-1
fi

echo -e $date_"\t"${WEEKLY_EXPS[0]}"\t"${WEEKLY_EXPS[1]}"\t"${WEEKLY_EXPS[2]}"\t"${WEEKLY_EXPS[3]}"\t"${WEEKLY_EXPS[4]}"\t"${WEEKLY_EXPS[5]}"\t"${WEEKLY_EXPS[6]}"\t"${WEEKLY_EXPS[7]}"\t"${WEEKLY_EXPS[8]}"\t"${WEEKLY_EXPS[9]}"\t"${WEEKLY_EXPS[10]} >> /home/dvctrader/midterm/Data_Generator/expiries_weekly.txt

#Generate the weekly options data filenames
rm FILENAMES
rm Generated_Data/*
touch /NAS1/data/NSELoggedData/NSE/$year/$month/$day/NSE_BANKNIFTY_*E_*
for i in `seq 0 10`; do
find /NAS1/data/NSELoggedData/NSE/$year/$month/$day/NSE_BANKNIFTY_*E_*_${WEEKLY_EXPS[$i]}_* -type f >> FILENAMES
done
if [[ ! -s FILENAMES ]];
then
	exit 2
fi

#Generate the data
cd ..
#/home/dvctrader/midterm/Data_Generator/historical_bar_generator_options_weekly
/home/dvctrader/cvquant_install/midterm/bin/nse_historical_data_generator_weeklyoptions
#Sort the generated data
cd Data_Generator/Generated_Data
for i in *; do sort -n -k 1 $i >> /home/dvctrader/midterm/Data_Generator/Sorted_Weekly_Options_Data/$i; done

