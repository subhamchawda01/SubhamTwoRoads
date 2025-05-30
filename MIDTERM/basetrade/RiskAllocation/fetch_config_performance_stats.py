#!/usr/bin/python

'''
\author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
Address:
    Suite No 353, Evoma, #14, Bhattarhalli,
    Old Madras Road, Near Garden City College,
    KR Puram, Bangalore 560049, India
    +91 80 4190 3551
'''

import sys
import os

import numpy as np
import pandas as pd

sys.path.append(os.path.expanduser('~/basetrade/'))
from sqlalchemy import create_engine
from datetime import datetime, date, timedelta, timezone
from pandas.tseries.offsets import BDay
import MySQLdb


def read_table(query):
    engine = create_engine("mysql+mysqldb://dvcreader:f33du5rB@52.87.81.158/results")
    data = None
    try:
        data = pd.read_sql_query(query, engine)
    except MySQLdb.Error as e:
        print(query)
        print(e)
    engine.dispose()
    return(data)


def fetch_config_eod_stats(list_of_configids, start_date, end_date, list_of_dates = None):
    eod_pnl_df = pd.DataFrame()
    daily_min_pnl_df = pd.DataFrame()
    daily_dd_df = pd.DataFrame()
    for t_cid in list_of_configids:
        query = "SELECT date, pnl, min_pnl, drawdown FROM wf_results WHERE configid = " + str(t_cid) + " AND date >= " + str(start_date) + " AND date <= " + str(end_date) + ";"
        df = read_table(query)
        df.set_index('date', inplace=True)
        eod_pnl_df = pd.concat([eod_pnl_df, df['pnl']], axis=1)
        daily_min_pnl_df = pd.concat([daily_min_pnl_df, df['min_pnl']], axis=1)
        daily_dd_df = pd.concat([daily_dd_df, df['drawdown']], axis=1)
    eod_pnl_df.fillna(value=np.nan, inplace=True)
    daily_min_pnl_df.fillna(value=np.nan, inplace=True)
    daily_dd_df.fillna(value=np.nan, inplace=True)
    eod_pnl_df.reset_index(inplace=True)
    daily_min_pnl_df.reset_index(inplace=True)
    daily_dd_df.reset_index(inplace=True)
    header = ['date']
    header.extend([str(id) for id in list_of_configids])
    eod_pnl_df.columns = header
    daily_min_pnl_df.columns = header
    daily_dd_df.columns = header
    eod_pnl_df.set_index('date', inplace=True)
    daily_min_pnl_df.set_index('date', inplace = True)
    daily_dd_df.set_index('date', inplace =True)   
    return (eod_pnl_df, daily_min_pnl_df, daily_dd_df)

    
def fetch_config_intra_day_pnl_series(list_of_configids, trade_date, cum=True):
    sdata = pd.DataFrame()
    for t_cid in list_of_configids:
        query = "SELECT date, pnl_samples FROM wf_results WHERE configid = " + str(t_cid) + " AND date >= " + str(trade_date) + " AND date <= " + str(trade_date) + ";"
        df = read_table(query)
        t_data = pd.DataFrame()
        if len(df) <= 0:
            t_data = pd.DataFrame(0, index=sdata.index, columns=[t_cid])
            sdata = pd.concat([sdata, t_data], axis=1)
        for i in range(len(df)):
            t_df = pd.concat([pd.DataFrame([float(str(df['date'][i]) + "." + str(x)) for x in df['pnl_samples'][i].split(" ")[0::2]]), pd.DataFrame(df['pnl_samples'][i].split(" ")[1::2])], axis=1)
            t_df.columns = ['Date', t_cid]
            t_df.set_index('Date', inplace=True)
            if cum is False:
                t_data = t_data.diff()
            t_data = pd.concat([t_data, t_df], axis=0)
            sdata = pd.concat([sdata, t_data], axis=1)
    # this logic works  if we are getting one date per call
    sdata.fillna(method='ffill', inplace=True)
    sdata.fillna(0, inplace=True)
    return(sdata)

def fetch_config_intra_day_pnl_series_over_days(list_of_configids, start_date, end_date, cum=True):
    start_trade_date = datetime.strptime(str(start_date), "%Y%m%d")
    end_trade_date = datetime.strptime(str(end_date), "%Y%m%d")
    trade_date = start_trade_date
    all_data = pd.DataFrame()
    while (trade_date.weekday() >= 5):
        trade_date = trade_date + BDay(1)
    while (trade_date <= end_trade_date):
        # we have to get one day each time, because for missing values,
        # we need to fill with zeros at the begining for the day and do ffill at the end
        daily_data = fetch_config_intra_day_pnl_series(list_of_configids, trade_date.strftime("%Y%m%d"), cum)
        all_data = pd.concat([all_data, daily_data])
        trade_date = trade_date + BDay(1)
    return(all_data)

def fetch_weighted_portfolio_daily_maxdd_over_days(portfolio, start_date, end_date):
    start_trade_date = datetime.strptime(str(start_date), "%Y%m%d")
    end_trade_date = datetime.strptime(str(end_date), "%Y%m%d")
    trade_date = start_trade_date
    all_data = pd.DataFrame()
    while (trade_date.weekday() >= 5):
        trade_date = trade_date + BDay(1)
    while (trade_date <= end_trade_date):
        # we have to get one day each time, because for missing values,
        # we need to fill with zeros at the begining for the day and do ffill at the end
        daily_data = fetch_weighted_portfolio_daily_maxdd(portfolio, trade_date.strftime("%Y%m%d"))
        all_data = pd.concat([all_data, daily_data])
        trade_date = trade_date + BDay(1)
    return(all_data)

def fetch_weighted_portfolio_daily_maxdd(portfolio, trade_date):   
    df = fetch_config_intra_day_pnl_series(list(portfolio.index), trade_date, True)
    max_dd = pd.DataFrame(data=[0], index=[trade_date], columns=["MaxDD"])
    if len(df) > 0:
        risk = pd.concat([pd.DataFrame(portfolio).T] * len(df))
        risk = risk.astype(int)
        risk.index = df.index
        risk.columns = df.columns
        df = df.astype(int)
        w_df = df * risk
        pnl_df = w_df.sum(axis=1)
        max_dd["MaxDD"][trade_date] = max(np.maximum.accumulate(pnl_df) - pnl_df)
    return(max_dd)

    
