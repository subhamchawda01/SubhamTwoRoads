#!/usr/bin/python
# -*- coding: utf-8 -*-
import os
import sys
import time
import pytz
import getpass
import argparse
import subprocess
import pandas as pd
import datetime as dt
from os import system
sys.path.append('/home/' + getpass.getuser() + '/basequant/hftrap/')
from Libraries.GetLotsize import LotsizeInit
lti = LotsizeInit('/spare/local/tradeinfo/NSE_Files/midterm_db')

COMPUTATION_SLEEP_TIME = 30
ORS_LOGS_DIR = '/spare/local/ORSlogs/NSE_FO/MSFO/'
FILE_EXEC_LOGS_DIR = '/spare/local/files/NSE/FileExecLogs/'
TCP_EXEC_LOGS_DIR = '/spare/local/files/NSE/ExecutionLogs/'
DISP_EXEC_LOGS_DIR = '/spare/local/files/NSE/DispersionExecLogs/'
WEEKLY_EXPIRIES = '/spare/local/files/NSE/MidTermLogs/ExpiryDates/weekly'
MONTHLY_EXPIRIES = '/spare/local/files/NSE/MidTermLogs/ExpiryDates/monthly'
MONITORING_FILE_PATH = '/home/dvctrader/important/MANUAL_TRADE/New_ExecutionTradesMatching'
DATASOURCE_EXCHANGE_SYMBOL_FILE = '/spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt'

file = open(MONITORING_FILE_PATH, 'w')
sys.stdout = file
sys.stderr = file

def BashExec(command):
    command = command.replace('&', '\&')
    return subprocess.check_output(['bash', '-c',
                                   command]).decode('UTF-8').rstrip()


def GetExpiryList(date_, file):
    expiries = BashExec("cat %s | awk \'$1 >= %s\'" % (file, date_))
    return expiries.split('\n')


def GetDataSourceSymbol(exch_sym, expiry_to_num_monthly,
                        expiry_to_num_weekly):
    datasource_symbol = BashExec("grep -w %s %s | awk \'{print $2}\'"
                                 % (exch_sym,
                                 DATASOURCE_EXCHANGE_SYMBOL_FILE))
    symbol_split = datasource_symbol.split('_')
    instrument = ''
    if '_PE_' in datasource_symbol or '_CE_' in datasource_symbol:
        instrument = '_'.join(symbol_split[1:3]) + '_' \
            + symbol_split[4] + '_'
        if symbol_split[3] in expiry_to_num_monthly:
            instrument += str(expiry_to_num_monthly[symbol_split[3]])
        else:
            instrument += str(expiry_to_num_weekly[symbol_split[3]]) + '_W'
    else:
        instrument = '_'.join(symbol_split[0:2]) + '_FUT' + str(expiry_to_num_monthly[symbol_split[3]])
    return instrument


def LoadORSTrades(file, expiry_to_num_monthly, expiry_to_num_weekly):
    try:
        df = pd.read_csv(file, delimiter='\001', usecols=range(3),
                         names=['EXCH_SYM', 'BS', 'Pos'])
        df['Pos'] = df.apply(lambda x: (-1 * x['Pos'] if x['BS']
                              == 1 else 1 * x['Pos']), axis=1)
        del df['BS']
        df = df.groupby('EXCH_SYM').agg({'Pos': 'sum'
                                        }).reset_index(drop=False)
        df['Instrument'] = df['EXCH_SYM'].apply(lambda x: \
                GetDataSourceSymbol(x, expiry_to_num_monthly,
                expiry_to_num_weekly))
        df = df[['Instrument', 'Pos']]
    except:
        print('warning : %s file not found'%(file))
        df = pd.DataFrame(columns=['Instrument', 'Pos'])
    return df


def LoadExecTrades(file, is_last_week):
    try:
        df = pd.read_csv(file, delim_whitespace=True, usecols=range(4),
                         names=['OrderId_Instrument', 'Shortcode',
                         'TradeType', 'Pos'])
        df['Instrument'] = df['OrderId_Instrument'].apply(lambda x: \
                '_'.join(x.split('_')[2:]))
        df = df[['Instrument', 'Pos']]
        df = df.groupby('Instrument').agg({'Pos': 'sum'}).reset_index()
        if is_last_week == True:
            df['Instrument'] = df['Instrument'].apply(lambda x: \
                    x.replace('_W', '') if x[-2:] == "_W" else x);
    except:
        print('warning : %s not found'%(file))
        df = pd.DataFrame(columns=['Instrument', 'Pos'])
    return df

def GetLotSize(date, ticker_, expnum):
    return int(lti.GetLotsize(date, ticker_, expnum))

def ConvertToLots(date, row):
    symbol_split = row['Instrument'].split('_')
    ticker = ''
    exp_num = -1
    if '_PE_' in row['Instrument'] or '_CE_' in row['Instrument']:
        if 'W' == symbol_split[-1]:
            exp_num = int(symbol_split[-2])
        else:
            exp_num = int(symbol_split[-1])
        ticker = symbol_split[0]
    else:
        exp_num = int(symbol_split[-1][-1]);
        ticker = symbol_split[1]
    lotsize = GetLotSize(date, ticker, exp_num)
    row['Pos_ors'] = row['Pos_ors'] / lotsize
    row['Pos_exec'] = row['Pos_exec'] / lotsize
    return row

def Print(row):
	print(" | %33s  | %10d  | %10d  | %10d  |"%(row['Instrument'], row['Pos_exec'], row['Pos_ors'], row['mismatch']))

def main():
    #while True:
        print(chr(27) + '[2j')
        print('\033c')
        print('\x1bc')
#        date = dt.datetime.today().strftime('%Y%m%d')
        #date = "20200319"
        date = sys.argv[1]
        #date = "20200324"
        print("date:: ")
        print(date)
        ors_trade_file = ORS_LOGS_DIR + 'trades.' + date
        weekly_expiries = GetExpiryList(date, WEEKLY_EXPIRIES)
        monthly_expiries = GetExpiryList(date, MONTHLY_EXPIRIES)
        expiry_to_num_monthly = {monthly_expiries[i] : i \
        		for i in range(0, len(monthly_expiries))}
        expiry_to_num_weekly = {weekly_expiries[i] : i \
        		for i in range(0, len(weekly_expiries))}
        is_last_week = (weekly_expiries[0] == monthly_expiries[0])
        tcp_exec_trade_file = TCP_EXEC_LOGS_DIR \
            + 'nse_complex_execlogic_trades_' + date + '.dat'
        file_exec_trade_file = FILE_EXEC_LOGS_DIR \
            + 'nse_complex_execlogic_trades_60_' + date + '.dat'
        disp_exec_trade_file = DISP_EXEC_LOGS_DIR \
            + 'nse_complex_execlogic_trades_60_' + date + '.dat'
        ors_trades = LoadORSTrades(ors_trade_file,
                                   expiry_to_num_monthly, expiry_to_num_weekly)
        exec_trades = pd.DataFrame(columns=['Instrument', 'Pos'])
        exec_trades = exec_trades.append(LoadExecTrades(tcp_exec_trade_file, is_last_week))
        exec_trades = exec_trades.append(LoadExecTrades(file_exec_trade_file, is_last_week))
        exec_trades = exec_trades.append(LoadExecTrades(disp_exec_trade_file, is_last_week))
       	exec_trades = exec_trades.groupby('Instrument').agg( \
       			{'Pos' : 'sum'}).reset_index(drop=False)
        trades_diff = pd.merge(exec_trades, ors_trades, on='Instrument'
                               , suffixes=['_exec', '_ors'], how='outer').fillna(0)
        trades_diff = trades_diff[trades_diff['Pos_exec']
                                  != trades_diff['Pos_ors']].fillna(0).reset_index(drop=True)
        trades_diff = trades_diff.apply(lambda x : ConvertToLots(date, x), axis = 1)
        trades_diff['mismatch'] = trades_diff['Pos_ors'] - trades_diff['Pos_exec']
        now_ = dt.datetime.utcnow().replace(tzinfo = pytz.utc)
        now_ = now_.astimezone(pytz.timezone('Asia/Calcutta'))
        print(now_.strftime("%d %b %H:%M:%S"))
        print("||||||||||||||||||||||||||||||||||")
        print("\033[91m {}\033[00m" .format("TRADES MISMATCH"))
        print("||||||||||||||||||||||||||||||||||\n")
        print(" | %32s   | %10s  | %10s  | %10s  |"%('Instrument', 'ExecLogic', 'ORS', 'MISMATCH'))
        trades_diff.apply(lambda x : Print(x), axis=1)
        sys.stdout.flush()
        #time.sleep(COMPUTATION_SLEEP_TIME)


if __name__ == '__main__':
    main()
