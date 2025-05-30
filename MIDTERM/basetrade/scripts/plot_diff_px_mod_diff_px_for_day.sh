#!/bin/bash

#TODO ADD TIME RANGE
USAGE1="$0 SHORTCODE SHORTCODE/PORT TIME_INTERVAL THRESHOLD DATE {NTP/NTP_ORD/BMF_EQ}"

if [ $# -lt 5 ] ;
then
    echo $USAGE1;
    exit ;
fi
SHORTCODE=$1;
DATE=$5;
PRICE_DATA_EXEC=$HOME/basetrade_install/bin/print_diff_px_mod_diff_px_for_day
TEMP_OUTPUT_FILE=/tmp/diff_px_mod_diff_px.dat



echo "$PRICE_DATA_EXEC $*" ;

$PRICE_DATA_EXEC $* > $TEMP_OUTPUT_FILE ;



gnuplot_command='
set xdata time;                    
set timefmt "%s";                  
set format x "%H:%M";
set grid;
plot '

extra_cmd=''
extra_cmd="$extra_cmd, '$TEMP_OUTPUT_FILE' using 1:2 with lines title 'DiffPx_ModDiffPx.$SHORTCODE.$DATE'"


echo "$gnuplot_command  ${extra_cmd:1};"  | gnuplot -persist


rm -f $TEMP_OUTPUT_FILE ;

