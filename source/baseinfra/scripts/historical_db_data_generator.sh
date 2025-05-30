#!/bin/bash


YYYY=2021
MM=08
MKT_DATA_DIR="/NAS1/data/NSELoggedData/NSE/$YYYY/$MM/"
#TEMP

'''
for DD in `ls $MKT_DATA_DIR` ; do
    echo "./update_db_daily_eod.sh ${YYYY}${MM}${DD}";
    ./update_db_daily_eod.sh ${YYYY}${MM}${DD}
done
'''
'''
./update_db_daily_eod.sh 20210901
./update_db_daily_eod.sh 20210902
./update_db_daily_eod.sh 20210903
./update_db_daily_eod.sh 20210906
./update_db_daily_eod.sh 20210907
./update_db_daily_eod.sh 20210908
./update_db_daily_eod.sh 20210909
./update_db_daily_eod.sh 20210913
./update_db_daily_eod.sh 20210914
./update_db_daily_eod.sh 20210915
./update_db_daily_eod.sh 20210916
./update_db_daily_eod.sh 20210917
./update_db_daily_eod.sh 20210920
./update_db_daily_eod.sh 20210921
./update_db_daily_eod.sh 20210922
./update_db_daily_eod.sh 20210923
#./update_db_daily_eod.sh 20210924
'''
MM=07
MKT_DATA_DIR="/NAS1/data/NSELoggedData/NSE/$YYYY/$MM/"
for DD in `ls $MKT_DATA_DIR` ; do
    echo "/NAS4/raghu/DB_Project/cvquant_install/baseinfra/bin/db_bhavcopy_techincalIndicator_update_for_day ${YYYY}${MM}${DD}";
    /NAS4/raghu/DB_Project/cvquant_install/baseinfra/bin/db_bhavcopy_techincalIndicator_update_for_day ${YYYY}${MM}${DD}
done

MM=08
MKT_DATA_DIR="/NAS1/data/NSELoggedData/NSE/$YYYY/$MM/"
for DD in `ls $MKT_DATA_DIR` ; do
    echo "/NAS4/raghu/DB_Project/cvquant_install/baseinfra/bin/db_bhavcopy_techincalIndicator_update_for_day ${YYYY}${MM}${DD}";
    /NAS4/raghu/DB_Project/cvquant_install/baseinfra/bin/db_bhavcopy_techincalIndicator_update_for_day ${YYYY}${MM}${DD}
done

MM=09
MKT_DATA_DIR="/NAS1/data/NSELoggedData/NSE/$YYYY/$MM/"
for DD in `ls $MKT_DATA_DIR` ; do
    echo "/NAS4/raghu/DB_Project/cvquant_install/baseinfra/bin/db_bhavcopy_techincalIndicator_update_for_day ${YYYY}${MM}${DD}";
    /NAS4/raghu/DB_Project/cvquant_install/baseinfra/bin/db_bhavcopy_techincalIndicator_update_for_day ${YYYY}${MM}${DD}
done
