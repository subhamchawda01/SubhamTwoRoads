#!/bin/bash

#TODO ADD TIME RANGE
USAGE1="$0 SHORTCODE SHORTCODE S_DATE E_DATE {NTP/NTP_ORD/BMF_EQ}"

if [ $# -lt 4 ] ;
then
    echo $USAGE1;
    exit ;
fi
SHORTCODE=$1;
S_DATE=$2;
E_DATE=$3;
VOL_DATA_EXEC="$HOME/basetrade_install/bin/mkt_trade_logger SIM"
TEMP_OUTPUT_FILE=/tmp/$SHORTCODE.vol


for date in 20140909 ; $BR_DOL_0 $date NTP | grep OnTradePrint | awk '{ vol+=$5 } END { if ( vol > 0 ) { print vol } } '

echo "$PRICE_DATA_EXEC $*" ;

$PRICE_DATA_EXEC $* > $TEMP_OUTPUT_FILE ;



gnuplot_command='
set xdata time;                    
set timefmt "%s";                  
set format x "%H:%M";
plot '

extra_cmd=''
extra_cmd="$extra_cmd, '$TEMP_OUTPUT_FILE' using 1:2 with lines title '$SHORTCODE'"


echo "$gnuplot_command  ${extra_cmd:1};"  | gnuplot -persist


rm -f $TEMP_OUTPUT_FILE ;

