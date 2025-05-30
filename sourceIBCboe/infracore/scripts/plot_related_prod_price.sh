#!/bin/bash


USAGE1="$0 SHORTCODE DATE";

if [ $# -lt 2 ] ;
then
    echo $USAGE1;
    exit;
fi


possible_dep=($1 `grep -w SOURCE ~/indicatorwork/prod_configs/comb_config_${1}* | cut -d" " -f 2-`)


DATA_EXEC=$HOME"/infracore_install/bin/mds_log_l1_mid_per_min" ;
#TMP_FILE="/tmp/mds_l1_mid_data."$1"_"$2;
#$DATA_EXEC $* > $TMP_FILE 

gnuplot_command='
set xdata time;                    
set timefmt "%s";                  
set format x "%H:%M"; 
plot "'
#plot "'$TMP_FILE'" using 1:2 with lines, "'

for i in ${possible_dep[*]}
do
    TMP_FILE="/tmp/mds_l1_mid_data."$i"_"$2
    if [ ! -e $TMP_FILE ]; then $DATA_EXEC $i $2 > /tmp/tttt; fi
    ms=(`awk '{sum += $2; sqsum += $2*$2}END{mn=sum/NR; mnsq=sqsum/NR; print mn, mnsq - mn*mn}' /tmp/tttt`)
    ms[1]=`echo "sqrt ( ${ms[1]} )" | bc`;
    echo "Running for "$i" >>> mean: "${ms[*]}
    cat /tmp/tttt | awk -v mean=${ms[0]} -v sd=${ms[1]}  '{print $1, ($2 - mean)/sd}' /tmp/tttt > $TMP_FILE
    gnuplot_command=${gnuplot_command}${TMP_FILE}'" using 1:2 with lines, "'
done

gnuplot_command=`echo $gnuplot_command | sed 's/, "$/;/'`
echo $gnuplot_command
echo $gnuplot_command  | gnuplot -persist

#rm /tmp/tttt
#rm /tmp/mds_l1_mid_data*
# set term postscript eps color;
# set output "/tmp/'$1'.eps";
# set style line 1 lt 1 lc 1;
# set style line 2 lt 1 lc 2;
# set style line 3 lt 1 lc 3;
# set style line 4 lt 1 lc 4;
# set style line 5 lt 1 lc 5;
# set style line 6 lt 1 lc 6;
# set style line 7 lt 1 lc 7;
# set style line 8 lt 1 lc 8;
# set style line 9 lt 1 lc 9;
# set style line 10 lt 1 lc 10;
# set style increment user;
