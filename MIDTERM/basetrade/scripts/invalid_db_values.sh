curr_date=`date --date="-1 day" +%y%m%d`
~/basetrade/pylib/mysql_fetch_rows.py -c "select * from results, strats where results.stratid = strats.stratid and last_update >= $curr_date and type='N' and 
( (supp_per < 0 or supp_per > 100) or 
( best_per < 0 or best_per > 100) or 
( agg_per < 0 or agg_per > 100) or 
( imp_per < 0 or imp_per > 100) or 
median_ttc < 0 or avg_ttc < 0 or
max_ttc < 0 or
drawdown < 0 or
msg_count < 0 or
vol_norm_avg_ttc < 0 or
otl_hits < 0 or
abs_open_pos < 0     or
uts < 0 or
ptrds < 0 or        
ttrds < 0 )" > /spare/local/logs/invalid_values
/home/dvctrader/infracore_install/bin/send_slack_notification test FILE /spare/local/logs/invalid_values
