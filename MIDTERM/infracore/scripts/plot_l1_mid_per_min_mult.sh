#!/bin/bash

USAGE1="$0 SHORTCODE [SHORTCODE ....] DATE";


#export NEW_GCC_LIB=/usr/local/lib
#export NEW_GCC_LIB64=/usr/local/lib64
#export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

if [ $# -lt 2 ] ;
then
    echo $USAGE1;
    exit;
fi
n=$(expr $# - 1)
date=$BASH_ARGV
t=($*)
prod=${t[*]:0:n}
echo ${prod[*]};
DATA_EXEC=$HOME"/infracore_install/bin/mds_log_l1_mid_per_min" ;
gnuplot_command='
set xdata time;                    
set timefmt "%s";                  
set format x "%H:%M";
plot '
extra_cmd=''
for p in ${prod[*]}
do
    echo $date $p
    TMP_FILE="/tmp/mds_l1_mid_data."$USER"_"$p"_"$date ;
    $DATA_EXEC $p $date > /tmp/tmp_mds.$USER
    $HOME/infracore/scripts/normalize.sh /tmp/tmp_mds.$USER 2 > $TMP_FILE
    extra_cmd="$extra_cmd, '$TMP_FILE' using 1:2 with lines title '$p'"
done

echo "$gnuplot_command  ${extra_cmd:1};"  | gnuplot -persist

for p in ${prod[*]}
do 
    rm "/tmp/mds_l1_mid_data."$USER"_"$p"_"$date ;
done


