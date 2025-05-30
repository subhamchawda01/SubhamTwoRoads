#!/bin/bash

YESTERDAY=$(~/basetrade_install/bin/calc_prev_week_day $(date  +%Y%m%d));

/home/dvctrader/basetrade_install/scripts/compare_pickstrats.pl $YESTERDAY > /spare/local/pickstrats_logs/compare_pickstrats_logs/log.$YESTERDAY 2>&1 ;

