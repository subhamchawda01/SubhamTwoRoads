#!/bin/bash
#args=("$@")

date_="$(date +'%Y%m%d')"
day_="$(date +'%A')"

cp /spare/local/tradeinfo/NSE_Files/midterm_db /home/dvctrader/trash/midterm_db_temp

#Update database with latest earnings daily
/apps/anaconda/anaconda3/bin/python /home/dvctrader/infracore/scripts/MidTermDB_EarningsUpdator.py

#Updates the PAIRS_ADF_STAT table for next week
if [[ "$day_" == "Friday" ]]; then
	next_date_=$date_
	for ((i=0;i<=4;i++)) do
		next_date_=$(/home/$USER/cvquant_install/infracore/bin/update_date $next_date_ N W 1)
		/apps/anaconda/anaconda3/bin/python /home/dvctrader/infracore/scripts/pair_adf_stat_updator.py /home/dvctrader/trash/midterm_db_temp $date_ $next_date_
	done
fi

mv /home/dvctrader/trash/midterm_db_temp /spare/local/tradeinfo/NSE_Files/midterm_db
