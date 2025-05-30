#!/bin/bash

#Main 
if [ $# -ne 1 ] ; then
  echo "Called As : " $* ;
  echo "$0 YYYYMMDD"
  exit;
fi


date_=$1

#rsync -avz 192.168.132.12:/spare/local/MDSlogs/GENERIC_NIFTY /spare/local/MDSlogs/
#rsync -avz 192.168.132.12:/spare/local/MDSlogs/GENERIC /spare/local/MDSlogs/

echo "Remove Options data if Logged Any"
mkdir -p /spare/local/MDSlogs/GENERIC
cd /spare/local/MDSlogs/GENERIC
rm *_PE_*
rm *_CE_*
echo "GENERIC NIFTY"
mkdir -p /spare/local/MDSlogs/GENERIC_NIFTY
cd /spare/local/MDSlogs/GENERIC_NIFTY
rm *_PE_*
rm *_CE_*
cd


cd /spare/local/MDSlogs/GENERIC
mv 'BSE_ATUL*_'${date_} BSE_ATUL_${date_}

for f_name in `ls | grep '\*_' | awk '{print $1}' | grep -v ATUL`; do
    new_f_name_=`echo $f_name | sed 's/\*//'`
    echo "mv $f_name $new_f_name_"
    mv $f_name "$new_f_name_"
     echo "HELLO"
done

echo "Running BSE Convertion"
export LD_LIBRARY_PATH=/apps/anaconda/anaconda3/lib; /home/pengine/prod//live_scripts/convert_sync_logged_data_bse.sh $date_
echo 
echo 
echo "Running NSE Convertion"
echo 
export LD_LIBRARY_PATH=/apps/anaconda/anaconda3/lib; /home/pengine/prod//live_scripts/convert_sync_logged_data_nse.sh $date_
