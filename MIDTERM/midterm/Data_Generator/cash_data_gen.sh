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


#Generate the cash data filenames
rm FILENAMES
rm Generated_Data/*
#touch /media/shared/ephemeral*/s3_cache/NAS1/data/NSELoggedData/NSE/$year/$month/$day/*
find /media/shared/ephemeral*/s3_cache/NAS1/data/NSELoggedData/NSE/$year/$month/$day/ -type f | grep -v _FUT_ | grep -v _CE_ | grep -v _PE_ >> FILENAMES
if [[ ! -s FILENAMES ]];
then
	exit 1
fi

#Generate the futures data
cd ..
/home/dvctrader/midterm/Data_Generator/historical_bar_generator_cash

#Sort the generated futures data
cd Data_Generator/Generated_Data
for i in *; do sort -n -k 1 $i >> /home/dvctrader/midterm/Data_Generator/Sorted_Cash_Data/$i; done
