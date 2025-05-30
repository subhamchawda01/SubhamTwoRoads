#!/bin/bash

Execs="get_utc_hhmm_str get_min_price_increment \
sim_strategy datagen timed_data_to_reg_data timed_data_to_multiple_reg_data \
remove_mean_reg_data get_dep_corr get_correlation_matrix get_stdev_correlation_matrix get_rep_matrix \
callFSLR callFSHLR callFSHDVLR callFSVLR callFSRR callMARS \
summarize_local_results_dir_and_choose_by_algo \
economic_events_of_the_day check_indicator_data get_avg_volume_for_shortcode"

if [ $# -gt 0 ];
then Execs="$*"
fi

DevMachines="10.23.199.51
10.23.199.52
10.23.199.53
10.23.199.54
10.23.199.55
10.23.142.51"

for exec in $Execs
do
    echo "Updating:" $exec;
    for machine in $DevMachines;
    do 
	echo "to: " $machine
	/home/dvctrader/basetrade/scripts/sync_exec_to_trade_machine.sh $machine $exec
#	/home/dvctrader/basetrade/scripts/sync_exec_to_all_trade_machines.sh $exec
    done
done


