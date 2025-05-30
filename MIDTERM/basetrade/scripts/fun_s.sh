#!/bin/bash
for i in FGBL FGBS FGBM FESX FDAX; 
do  
    ~/basetrade/scripts/rank_hist_queries.pl US ${i}_0 20120601 TODAY > ~/result_ind_population/rank_hist_$i
    exec<~/result_ind_population/rank_hist_$i
    rm ~/result_ind_population/res_${i}_new
    while read line
    do
	echo $line>> ~/result_ind_population/res_${i}_new
	st=`echo $line | cut -d' ' -f 3`
	echo $i $st
	find_ind_performance.py ~/modelling/strats/*/*/$st 20120501 20120718 >> ~/result_ind_population/res_${i}_new;
    done
done
