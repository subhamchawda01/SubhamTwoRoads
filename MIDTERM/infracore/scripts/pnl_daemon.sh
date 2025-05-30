#! /bin/bash

YYYYMMDD=`date +"%Y%m%d"` ;
TOMORROW_DATE=`date --date='tomorrow' +"%Y%m%d"` ;
mkdir -p /spare/local/logs/pnl_data/hft/delta_$YYYYMMDD
cp -R -f /spare/local/logs/pnl_data/hft/delta_files/* /spare/local/logs/pnl_data/hft/delta_$YYYYMMDD 2>>/spare/local/logs/pnl_data/hft/pnl_log
rm /spare/local/logs/pnl_data/hft/delta_files/* ;
cat /spare/local/logs/pnl_data/hft/pnls.txt > /spare/local/logs/pnl_data/hft/pnl_log.txt ; 
> /spare/local/logs/pnl_data/hft/pnls.txt ;
perl /home/dvcinfra/LiveExec/scripts/calc_ors_pnl.pl 'C' 'H' $TOMORROW_DATE > /spare/local/logs/pnl_data/hft/pnls.txt 2>>/spare/local/logs/pnl_data/hft/pnl_log &

mkdir -p /spare/local/logs/pnl_data/mtt/delta_$YYYYMMDD
cp -R -f /spare/local/logs/pnl_data/mtt/delta_files/* /spare/local/logs/pnl_data/mtt/delta_$YYYYMMDD 2>>/spare/local/logs/pnl_data/mtt/pnl_log
rm /spare/local/logs/pnl_data/mtt/delta_files/* ;
cat /spare/local/logs/pnl_data/mtt/pnls.txt > /spare/local/logs/pnl_data/mtt/pnl_log.txt ; 
> /spare/local/logs/pnl_data/mtt/pnls.txt ;
perl /home/dvcinfra/LiveExec/scripts/calc_ors_pnl.pl 'C' 'M' $TOMORROW_DATE > /spare/local/logs/pnl_data/mtt/pnls.txt 2>>/spare/local/logs/pnl_data/mtt/pnl_log &
