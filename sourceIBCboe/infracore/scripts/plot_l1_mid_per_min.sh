#!/bin/bash

USAGE1="$0 SHORTCODE DATE STARTTIME ENDTIME";


export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

if [ $# -lt 2 ] ;
then
    echo $USAGE1;
    exit;
fi

DATA_EXEC=$HOME"/infracore_install/bin/mds_log_l1_mid_per_min" ;
TMP_FILE="/tmp/mds_l1_mid_data."$1"_"$2"_"$3"_"$4 ;

$DATA_EXEC $* > $TMP_FILE 

gnuplot_command='
set xdata time;                    
set timefmt "%s";                  
set format x "%H:%M";              
plot "'$TMP_FILE'" using 1:2 with lines; '

echo $gnuplot_command  | gnuplot -persist

rm $TMP_FILE

