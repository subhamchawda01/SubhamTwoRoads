#!/bin/bash

#TODO ADD TIME RANGE
USAGE1="$0 SHORTCODE SHORTCODE/PORT TIME_INTERVAL DATE {NTP/NTP_ORD/BMF_EQ}"

if [ $# -lt 4 ] ;
then
    echo $USAGE1;
    exit ;
fi
SHORTCODE=$1;
DATE=$4;
PRICE_DATA_EXEC=$HOME/basetrade_install/bin/print_trend_mean_reversion_ind_for_day
TEMP_OUTPUT_FILE=/tmp/trend_mean_reversion_ind_data.dat



echo "$PRICE_DATA_EXEC $*" ;

$PRICE_DATA_EXEC $* > $TEMP_OUTPUT_FILE ;



gnuplot_command='
set xdata time;                    
set timefmt "%s";                  
set format x "%H:%M";
plot '

extra_cmd=''
extra_cmd="$extra_cmd, '$TEMP_OUTPUT_FILE' using 1:2 with lines title '$SHORTCODE.$DATE'"


echo "$gnuplot_command  ${extra_cmd:1};"  | gnuplot -persist


rm -f $TEMP_OUTPUT_FILE ;

