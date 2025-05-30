# ~/infracore/scripts/set_live_exec.sh tradeinit
~/infracore/scripts/set_live_exec.sh get_utc_hhmm_str
~/infracore/scripts/set_live_exec.sh get_min_price_increment
~/infracore/scripts/set_live_exec.sh sim_strategy
~/infracore/scripts/set_live_exec.sh datagen
~/infracore/scripts/set_live_exec.sh timed_data_to_reg_data
~/infracore/scripts/set_live_exec.sh remove_mean_reg_data
~/infracore/scripts/set_live_exec.sh get_dep_corr
~/infracore/scripts/set_live_exec.sh get_correlation_matrix
~/infracore/scripts/set_live_exec.sh get_stdev_correlation_matrix
~/infracore/scripts/set_live_exec.sh get_rep_matrix
~/infracore/scripts/set_live_exec.sh callFSLR
~/infracore/scripts/set_live_exec.sh callFSHLR
~/infracore/scripts/set_live_exec.sh callFSVLR
~/infracore/scripts/set_live_exec.sh callFSRR
~/infracore/scripts/set_live_exec.sh summarize_strategy_results
~/infracore/scripts/set_live_exec.sh summarize_single_strategy_results
~/infracore/scripts/set_live_exec.sh summarize_local_results_and_choose
~/infracore/scripts/set_live_exec.sh summarize_local_results_and_choose_by_sharpe

find ~/LiveExec -name *~ -type f -mtime +10 -exec rm -f {} \;

