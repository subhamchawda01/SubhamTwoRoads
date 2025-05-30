#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import sys
import trlibs
import getpass
import subprocess
import pandas as pd
import datetime as dt
from os import listdir
from os.path import isfile, join
from pandas.tseries.offsets import BDay
sys.path.append(os.path.abspath('/home/' + getpass.getuser() + '/hftrap/'))

os.environ['LD_LIBRARY_PATH'] = '/opt/glibc-2.14/lib'
slack_channel = 'nseinfo'
slack_data_mode = 'DATA'
slack_exec = '/home/pengine/prod/live_execs/send_slack_notification'
tmp_lot_file = '/home/dvctrader/trash/tmp_lot_file.csv'
monthly_expiries = '/spare/local/files/ExpiryDates/monthly'
corp_events_dir_ = '/NAS1/data/NSEMidTerm/MachineReadableCorpAdjustmentFiles/'


def BashExec(command):
    return subprocess.check_output(['bash', '-c',command]).decode('utf-8')

def GetMonthlyExpiry(date_):
    monthly_expiry_ = BashExec("cat %s | awk \'$1 >=%s\' | head -n1"
                               % (monthly_expiries, date_))
    return monthly_expiry_

def GetDatetimeFromYYYYMMDD(YYYYMMDD):
    return dt.datetime.strptime(YYYYMMDD, '%Y%m%d')

def SlackNotify(data):
    command = slack_exec + ' ' + slack_channel + ' ' + slack_data_mode + ' \"' + data  + "\""
    BashExec(command)

def GetNewLotDf(date):
    yyyy_ = date[0:4]
    corp_events_path_ = corp_events_dir_ + yyyy_
    corp_files = [x for x in listdir(corp_events_path_)
                  if int(x.split('_')[-1]) == int(date)]
    ticker_new_lot_df = pd.DataFrame(columns=['NEW_MARKET_LOT'])
    for file in corp_files:
        ticker = file.split('_')[0]
        details = pd.read_csv(join(corp_events_path_, file, 'Details.csv'), header=None)
        details = details.set_index(0)
        if 'NEW_MARKET_LOT' in details.index:
            ticker_new_lot_df.loc[ticker, 'NEW_MARKET_LOT'] = details.loc['NEW_MARKET_LOT'].iloc[0]
    return ticker_new_lot_df

def UpdateLotSize(row, ticker_new_lot_df):
    ticker = row[1].rstrip()
    slack_data = ''
    if ticker in ticker_new_lot_df.index:
        new_lot_size = ticker_new_lot_df.loc[ticker].iloc[0];
        slack_data = 'Lotsize updated for '+ ticker + ' old lotsize : '+ str(row[2]) + ' new lotsize : ' + str(new_lot_size);
        for i in range(2, len(row)):
            if str(row[i]).rstrip() != '':
                row[i] = int(new_lot_size);
            else:
                break
    for i in range(2, len(row)):
        row[i] = str(row[i]).ljust(11, ' ')
    if slack_data != '':
        SlackNotify(slack_data)
    return row

def main():
    today_ = dt.datetime.today().strftime('%Y%m%d')
    if trlibs.IsHoliday(today_,'NSE'):
        print ('NSE holiday, Exiting...');
        exit(0);
    next_working_day = trlibs.GetNextWorkingDay(today_,'NSE')
    monthly_expiry_ = GetMonthlyExpiry(next_working_day)
    monthly_lotsize_file_ = '/spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_' \
                        + monthly_expiry_[4:6] + monthly_expiry_[2:4] + '.csv'
    ticker_new_lot_df = GetNewLotDf(next_working_day)
    if len(ticker_new_lot_df) == 0:
        print ('No corp events for today, Exiting...')
        sys.exit(0)
    if os.path.isfile(tmp_lot_file):
        BashExec('rm ' + tmp_lot_file)
    ticker_lot_df = pd.read_csv(monthly_lotsize_file_)
    ticker_lot_df.fillna('',inplace = True)
    ticker_lot_df = ticker_lot_df.apply((lambda x: UpdateLotSize(x, ticker_new_lot_df)),axis=1)
    ticker_lot_df.to_csv(tmp_lot_file, index=False)
    BashExec('mv %s %s' % (tmp_lot_file, monthly_lotsize_file_))

if __name__ == '__main__':
    main()

