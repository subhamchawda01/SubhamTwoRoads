#!/bin/bash

if [ $# -lt 6 ]; then 
  echo "USAGE: $0 shc_ sd_ ed_ start_hhmm_ end_hhmm_ outfile_ [msecs_timeout_=3000] [use_owp=0]";
  exit 0;
fi;


shc=$1; 
sd=$2;
ed=$3;
shhmm=$4;
ehhmm=$5;
out=$6;
msecs=3000;
use_owp=0;
if [ $# -ge 7 ]; then msecs=$7; fi;
if [ $# -ge 8 ]; then use_owp=$8; fi;
uid=`date +%N`;
tmpfile=tmp_"$uid";

rm -f $tmpfile;
echo "MODELINIT DEPBASE NONAME MktSizeWPrice MktSizeWPrice" >> $tmpfile;
echo "MODELMATH LINEAR CHANGE" >> $tmpfile;
echo "INDICATORSTART" >> $tmpfile;
echo "INDICATOR 1.0 SimplePriceType $shc MidPrice" >> $tmpfile;
echo "INDICATOR 1.0 SimplePriceType $shc MktSizeWPrice" >> $tmpfile;
echo "INDICATOR 1.0 SimplePriceType $shc MktSinusoidal" >> $tmpfile;
#echo "INDICATOR 1.0 SimplePriceType $shc TradeWPrice" >> $tmpfile;
if [ $use_owp -gt 0 ]; then echo "INDICATOR 1.0 SimplePriceType $shc OrderWPrice" >> $tmpfile; fi;
echo "INDICATOREND" >> $tmpfile;
$HOME/basetrade/ModelScripts/generate_timed_indicator_data.pl $tmpfile $sd $ed $shhmm $ehhmm $msecs 0 0 $out;
rm -f $tmpfile;
