#!/bin/bash

USAGE="$0 DATE NEWPRODUCT(TATACONSUM)";

if [ $# -ne 2 ] ;
then
    echo $USAGE
    exit;
fi


date=$1
sym=$2;

for pe in _FUT0 _FUT1 _C0_I3 _C0_I2 _C0_I1 _C0_A _C0_O1 _C0_O2 _C0_O3; do 
  echo "FOR SHORTCODE-- NSE_${sym}${pe}"
#  /home/pengine/prod/live_execs/get_exchange_symbol NSE_${sym}${pe} $date

  grep `/home/pengine/prod/live_execs/get_exchange_symbol NSE_${sym}${pe} $date` /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt ; 
done

echo -e "\n**PE**\n"
for pe in _P0_I3 _P0_I2 _P0_I1 _P0_A _P0_O1 _P0_O2 _P0_O3; do 
  echo "FOR SHORTCODE-- NSE_${sym}${pe}"
#  /home/pengine/prod/live_execs/get_exchange_symbol NSE_${sym}${pe} $date

  grep `/home/pengine/prod/live_execs/get_exchange_symbol NSE_${sym}${pe} $date` /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt ; 
done
