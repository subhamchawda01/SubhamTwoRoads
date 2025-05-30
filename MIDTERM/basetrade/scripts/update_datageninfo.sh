#!/bin/bash

shc=`cat /spare/local/tradeinfo/datageninfo/prod_list.txt`;
#pshc= `grep -v "^$\|#" /spare/local/tradeinfo/traded_prod_list.txt`;

# bad periods #
/home/dvctrader/basetrade_install/scripts/get_bad_periods.pl $shc /spare/local/tradeinfo/datageninfo 400 2>/dev/null ;

# bad days && very bad days #
/home/dvctrader/basetrade_install/scripts/get_bad_days.pl $shc /spare/local/tradeinfo/datageninfo 400 120 2>/dev/null ;

# STDEV Calculation
/home/dvctrader/basetrade/scripts/compute_all_stdev.sh

#updating average l1event & trade counts used in EVT
/home/dvctrader/basetrade_install/scripts/update_avg_event_count_files.py /spare/local/tradeinfo/datageninfo/prod_list.txt 

#rsync datageninfo to all dev servers 
/home/dvctrader/infracore_install/scripts/sync_dir_to_all_dev_machines.pl /spare/local/tradeinfo/datageninfo

# high range days #
#/home/dvctrader/basetrade_install/scripts/get_high_range_days.pl $shc /spare/local/tradeinfo/datageninfo 400 120 2>/dev/null ;

# high volume days #
#/home/dvctrader/basetrade_install/scripts/get_high_volume_days.pl $shc /spare/local/tradeinfo/datageninfo 400 120 2>/dev/null ;

# low volume days #
#/home/dvctrader/basetrade_install/scripts/get_low_volume_days.pl $shc /spare/local/tradeinfo/datageninfo 400 120 2>/dev/null ;

# high(low) correlation periods #
#/home/dvctrader/basetrade/scripts/compute_corr_periods.sh

# Thin Book Periods
#/home/dvctrader/basetrade/scripts/compute_all_thin_book_periods.sh

