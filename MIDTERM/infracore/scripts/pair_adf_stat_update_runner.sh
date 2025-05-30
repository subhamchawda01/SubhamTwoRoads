#!/bin/bash
#args=("$@")

if [[ $# -ne 2 ]]; then
	echo "USAGE: bash /home/dvctrader/infracore/scripts/pair_adf_stat_update_runner.sh <source_date> <target_date>"
	echo "EXAMPLE: bash /home/dvctrader/infracore/scripts/pair_adf_stat_update_runner.sh 20170108 20170109"
	exit
fi

source_date_=$1
target_date_=$2

cp /spare/local/tradeinfo/NSE_Files/midterm_db /home/dvctrader/trash/midterm_db_temp
python /home/dvctrader/infracore/scripts/pair_adf_stat_updator.py /home/dvctrader/trash/midterm_db_temp $source_date_ $target_date_
mv /home/dvctrader/trash/midterm_db_temp /spare/local/tradeinfo/NSE_Files/midterm_db
