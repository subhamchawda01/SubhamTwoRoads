#!/bin/bash


print_msg_and_exit() {
  echo $* ;
  exit ;
}

[ $# -eq 1 ] || print_msg_and_exit "Usage : < script > <prod[SBIN]>"

#prod="INDIACEM"
#prod="JKCEMENT"
prod=$1
theo_file="/tmp/theo_file_append"
>$theo_file

for LIVE_DIR in `grep LIVE /home/dvctrader/EOD_SCRIPTS/eod_jobs.sh | awk '{print $7}' | sort -u | awk -F'LIVE_FILE.csv' '{print $1}'`;
do 
  echo "****** $LIVE_DIR ******"

#LIVE_DIR="/home/dvctrader/CONFIG_SET/CONFIG_FUT1_RATIO_CALCULATOR/"
  cd $LIVE_DIR
#  f1=""
#  f2=""
  if [ `ls | grep $prod | wc -l` -gt 0 ];
  then
    echo "$prod related files are already present under $LIVE_DIR"
  else
    if [ `echo $LIVE_DIR | grep _CM_ | wc -l` = "1" ];
    then 
      sbin_1="NSE_SBIN_SQUAREOFF"
      sbin_2="NSE_SBIN_MM"
      f1="NSE_${prod}_SQUAREOFF"
      f2="NSE_${prod}_MM"
    else
      sbin_1="NSE_SBIN_FUT1_SQUAREOFF"
      sbin_2="NSE_SBIN_FUT1_MM"
      f1="NSE_${prod}_FUT1_SQUAREOFF"
      f2="NSE_${prod}_FUT1_MM"
    fi
    echo "cp -R $sbin_1 $f1"
    cp -r $sbin_1 $f1
    echo "cp -R $sbin_2 $f2"
    cp -r $sbin_2 $f2

    echo "sed -i 's/SBIN/${prod}/g' ${f1}/*"
    sed -i "s/SBIN/${prod}/g" ${f1}/*
    echo "sed -i 's/SBIN/${prod}/g' ${f2}/*"
    sed -i "s/SBIN/${prod}/g" ${f2}/*

    echo -e "THEO1 = ${f1}\nTHEO2 = ${f2}" 
  fi

  echo -e "\n***********************\n"
done

