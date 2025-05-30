#!/usr/bin/python   
# -*- coding: utf-8 -*-
import pandas as pd
import datetime as dt
import os
import sys
import subprocess
from os import listdir
from os.path import isfile, join

ticker_new_lot_file = "new_outfile.txt";

def BashExec(command):
    return subprocess.check_output(['bash', '-c', command])

def SendSlackAlert(data):
    command = 'LD_LIBRARY_PATH=/opt/glibc-2.14/lib '+slack_exec+" "+channel+" DATA \""+data+"\"";
    BashExec(command);
#format format of lotsize file
#UNDERLYING                          ,SYMBOL    ,OCT-18     ,NOV-18     ,DEC-18     ,MAR-19     ,JUN-19     ,SEP-19     ,DEC-19     ,JUN-20     ,DEC-20     ,JUN-21     ,DEC-21     ,JUN-22     ,DEC-22     ,JUN-23     ,           
#NIFTY BANK                          ,BANKNIFTY ,40         ,40         ,40         ,           ,           ,           ,           ,           ,           ,           ,           ,           ,           ,           ,           
#ADANI ENTERPRISES LIMITED           ,ADANIENT  ,4000       ,4000       ,4000       ,           ,           ,           ,           ,           ,           ,           ,           ,           ,           ,           ,           
def UpdateDailyMonthlyLotFile(ticker_lot_df, monthly_lotsize_file_,daily_lotsize_file_):
	if ticker_lot_df.empty:
		return;
	ticker_lot_df.to_csv(ticker_new_lot_file,header=False,index=False);
	command1 = r'''awk 'BEGIN{FS=",";}{if(NF==2){map[$1]=$2}else{{symbol=$2;gsub(/[ ,]+/,"",symbol)}if(map[symbol]!="" && map[symbol]!="nan"){for(i=1;i<=NF;i++){if(i==1){printf "%-36s,",$1}else if(i==2){printf "%-10s,",$2}else if(i<=5){printf "%-11s,",map[symbol]}else if(i==NF){printf "%-11s\n",$i}else{printf "%-11s,",$i}}}else{print $0}}}'''+'\' '+ticker_new_lot_file+' '+daily_lotsize_file_ + ' > ' +tmp_daily_lotsize_file_; 
	command2 = r'''awk 'BEGIN{FS=",";}{if(NF==2){map[$1]=$2}else{{symbol=$2;gsub(/[ ,]+/,"",symbol)}if(map[symbol]!="" && map[symbol]!="nan"){for(i=1;i<=NF;i++){if(i==1){printf "%-36s,",$1}else if(i==2){printf "%-10s,",$2}else if(i<=5){printf "%-11s,",map[symbol]}else if(i==NF){printf "%-11s\n",$i}else{printf "%-11s,",$i}}}else{print $0}}}'''+'\' '+ticker_new_lot_file+' '+monthly_lotsize_file_+ ' > ' + tmp_monthly_lotsize_file_;
	BashExec(command1);
	BashExec(command2);
	BashExec("cp %s %s"%(tmp_daily_lotsize_file_,daily_lotsize_file_));
	BashExec("cp %s %s"%(tmp_monthly_lotsize_file_,monthly_lotsize_file_));

def GetTickerNewLotFromCorpEvents(curr_date):
    yyyy_ = curr_date[0:4]
    corp_events_path_ = corp_events_dir_ + yyyy_
    corp_files_ = [x for x in listdir(corp_events_path_) if int(x.split('_')[-1]) == int(curr_date)]
    ticker_lot_df_ = pd.DataFrame(columns=['New Lotsize'])
    for file in corp_files_:
        details = pd.read_csv(join(corp_events_path_, file,
                              'Details.csv'), header=None)
        details = details.set_index(0);
        ticker_ = file.split('_')[0];
        if 'NEW_MARKET_LOT' in details.index:
            if details.loc['NEW_MARKET_LOT'].iloc[0] != details.loc['OLD_MARKET_LOT'].iloc[0]:
                ticker_lot_df_.loc[ticker_,'New Lotsize'] = str(details.loc['NEW_MARKET_LOT'].iloc[0]);
                SendSlackAlert("Lot size updated for %s,new lot %s"%(ticker_,ticker_lot_df_.loc[ticker_,'New Lotsize']));
        else:
            ticker_lot_df_.loc[ticker_,'New Lotsize'] = 'nan'
            SendSlackAlert("Lot size not updated for %s, update manually"%(ticker_));
    ticker_lot_df_.reset_index(inplace = True);
    return ticker_lot_df_

#script will be called from fecth_nse_daily.sh with next working day as argument, so curr_date will be today+1
if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('provide date in the argument')
        sys.exit(-1);
    curr_date = sys.argv[1];
    corp_events_dir_ = '/NAS1/data/NSEMidTerm/MachineReadableCorpAdjustmentFiles/'
    daily_lotsize_file_ = '/spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_'+curr_date+'.csv';
    monthly_lotsize_file_ = '/spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_'+curr_date[4:6]+curr_date[2:4]+'.csv'
    tmp_daily_lotsize_file_ = '/home/dvctrader/tmp_fo_mktlots_' + curr_date + '.csv';
    tmp_monthly_lotsize_file_ = '/home/dvctrader/tmp_fo_mktlots_' + curr_date[4:6] + curr_date[2:4] + '.csv';
    slack_exec = '/home/pengine/prod/live_execs/send_slack_notification';
    channel = 'nsehft-alerts';
    ticker_new_lot_df = GetTickerNewLotFromCorpEvents(curr_date);
    UpdateDailyMonthlyLotFile(ticker_new_lot_df,monthly_lotsize_file_, daily_lotsize_file_);
