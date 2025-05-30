#!/bin/bash

#Main 
if [ $# -ne 1 ] ; then
  echo "Called As: YYYYMMDD" 
  exit
fi
#DELTA STRAT RUN
YYYYMMDD=$1
start_date=`date -d "$YYYYMMDD -40 days" "+%Y%m%d"`
echo "StartDate: $start_date EndDate: $YYYYMMDD"

cd /home/dvctrader/MFT_Analysis/
/home/dvctrader/MFT_Analysis/Analysis/BTtools/Backtests/OSM/batch_runner_deltastrat.sh NIFTY $start_date $YYYYMMDD
/home/dvctrader/MFT_Analysis/Analysis/BTtools/Backtests/OSM/batch_runner_deltastrat.sh BANKNIFTY $start_date $YYYYMMDD

echo "Dump Lot file"
/apps/anaconda/anaconda3/envs/py3/bin/python /home/dvctrader/MFT_Analysis/Analysis/BTtools/Backtests/OSM/MomentumOptionSellAnalysis_spreads_dump_nlots.py --startdate $start_date --enddate $YYYYMMDD --ticker NIFTY
/apps/anaconda/anaconda3/envs/py3/bin/python /home/dvctrader/MFT_Analysis/Analysis/BTtools/Backtests/OSM/MomentumOptionSellAnalysis_spreads_dump_nlots.py --startdate $start_date --enddate $YYYYMMDD --ticker BANKNIFTY


'''
exit
exit
#Live Runner 
cd /home/dvctrader/MFT_Analysis/
/home/dvctrader/MFT_Analysis/Strategy_Runners/SmartvolsurfaceGenerator/Live_Runner.py --startdate $start_date --enddate $YYYYMMDD --ticker NFW 
/home/dvctrader/MFT_Analysis/Strategy_Runners/SmartvolsurfaceGenerator/Live_Runner.py --startdate $start_date --enddate $YYYYMMDD --ticker NFM
/home/dvctrader/MFT_Analysis/Strategy_Runners/SmartvolsurfaceGenerator/Live_Runner.py --startdate $start_date --enddate $YYYYMMDD --ticker BNFW
/home/dvctrader/MFT_Analysis/Strategy_Runners/SmartvolsurfaceGenerator/Live_Runner.py --startdate $start_date --enddate $YYYYMMDD --ticker BNFM

cd /home/dvctrader/MFT_Analysis/
/home/dvctrader/MFT_Analysis/Analysis/BTtools/Backtests/OSM/batch_runner.sh $YYYYMMDD NIFTY
/home/dvctrader/MFT_Analysis/Analysis/BTtools/Backtests/OSM/batch_runner.sh $YYYYMMDD BANKNIFTY
'''
