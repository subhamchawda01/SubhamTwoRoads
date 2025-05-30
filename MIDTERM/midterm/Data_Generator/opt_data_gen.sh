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

cd /home/dvcinfra/midterm/Data_Generator/

#Update the expiry file
next_date=$( /home/dvcinfra/cvquant_install/dvccode/bin/update_date $date_ N W 1)
data=($(cat /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_date | grep NIFTY | grep -v BANK ) )

counter=0
while [ ${#data[@]} -eq 0 ] && [ $counter -lt 7 ];
do
	next_date=$( /home/dvcinfra/cvquant_install/dvccode/bin/update_date $next_date N W 1)
	data=($(cat /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_date | grep NIFTY | grep -v BANK ) )
	counter=$(( $counter + 1 ))
done
echo ${#data[@]}
EXPS=($(echo ${data[*]} | awk '{print $6" "$12" "$18} '))
if [ ${#EXPS[@]} -eq 0 ]
then
	exit 1
fi
IFS=$'\n'
EXPS=($(sort <<<"${EXPS[*]}"))
unset IFS
echo -e $date_"\t"${EXPS[0]}"\t"${EXPS[1]}"\t"${EXPS[2]} >> /home/dvcinfra/midterm/Data_Generator/expiries.txt
#Generate the futures data filenames
rm FILENAMES
rm Generated_Data/*
touch /NAS1/data/NSELoggedData/NSE/$year/$month/$day/NSE_*_*E_*
find /run/media/dvcinfra/NSEData/NSELoggedData/NSE/$year/$month/$day/NSE_*_*E_*_${EXPS[0]}_* -type f >> FILENAMES
find /run/media/dvcinfra/NSEData/NSELoggedData/NSE/$year/$month/$day/NSE_*_*E_*_${EXPS[1]}_* -type f >> FILENAMES
find /run/media/dvcinfra/NSEData/NSELoggedData/NSE/$year/$month/$day/NSE_*_*E_*_${EXPS[2]}_* -type f >> FILENAMES

if [[ ! -s FILENAMES ]]; then
	exit 2
fi

#Generate the futures data
cd ..
#/home/dvcinfra/midterm/Data_Generator/historical_bar_generator_options
#/home/dvcinfra/midterm/Data_Generator/Execs/nse_historical_data_generator_options
/home/dvcinfra/cvquant_install/midterm/bin/nse_historical_data_generator_options
#Sort the generated futures data
cd Data_Generator/Generated_Data
for i in *; do sort -n -k 1 $i >> /home/dvcinfra/midterm/Data_Generator/Sorted_Options_Data/$i; done

