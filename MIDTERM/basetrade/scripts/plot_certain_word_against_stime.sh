#!/bin/bash

#TODO ADD TIME RANGE
USAGE1="$0 DATA_INPUT_FILEPATH WORD KEY_TO_PLOT_X KEY_TO_PLOT_Y"
EXAMP1="$0 /spare/local/logs/tradelogs/log.20130206.1717 GlobalPNL: 1 4" 

if [ $# -lt 4 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit ;
fi

DATA_INPUT_FILEPATH=$1;
GREPKEY=$2;
KEY_TO_PLOT_X=$3;
KEY_TO_PLOT_Y=$4;

if [ -e $DATA_INPUT_FILEPATH ] ; then 
    mkdir -p $HOME"/tmp";
    
    TEMP_OUTPUT_FILE=$HOME"/tmp/temp_file_for_plot_certain_word_against_stime.txt";
    
    grep "$GREPKEY" $DATA_INPUT_FILEPATH > $TEMP_OUTPUT_FILE ;
    
    gnuplot_command='
set xdata time; 
set timefmt "%s"; 
set format x "%H:%M"; 
set grid; 
plot ';

    extra_cmd=''
    extra_cmd="$extra_cmd, '$TEMP_OUTPUT_FILE' using $KEY_TO_PLOT_X:$KEY_TO_PLOT_Y with lines title 'GK' ";
    
    echo "$gnuplot_command  ${extra_cmd:1};" | gnuplot -persist ;
    
    rm -f $TEMP_OUTPUT_FILE;
    
fi