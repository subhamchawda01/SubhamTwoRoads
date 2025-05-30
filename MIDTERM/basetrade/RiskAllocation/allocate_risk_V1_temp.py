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
import argparse
import subprocess

import pandas as pd
import numpy as np
import scipy as sp
from scipy.optimize import minimize

from datetime import datetime, date, timedelta, timezone
from pandas.tseries.offsets import BDay
sys.path.append(os.path.expanduser('~/basetrade/'))
from walkforward.wf_db_utils.db_handles import connection
from walkforward.wf_db_utils.fetch_config_details import fetch_configs_id, fetch_configs_strat_type
from RiskAllocation.fetch_config_performance_stats import fetch_config_eod_stats


# give a list_of_configs / one_dir [strategies that are available]
# give a date, d for which we are allocating risk [trade_date]
# given expected_pnl_series [called pick_strat_algo :(, need to change DVC lingo], i.e expected[mean/stdev], a frontier curve from MPT
# individual performance cutoffs
# min max allocation risk [0 - 100] [x - y]
def read_configs(filename = None, dirname = None):    
    config_list = []
    unit_risk_scale = []
    if filename is None:
        print(dirname)
    elif dirname is None:
        with open(filename, 'r') as fh:
            for line in fh:
                t_tokens = line.strip().split(" ")
                config_list.append(t_tokens[0])
                if len(t_tokens) > 1:
                    unit_risk_scale.append(t_tokens[1])
                else:
                    unit_risk_scale.append(1)
            fh.close()
    else:
        print("nothing to read")
    return(config_list, unit_risk_scale)

#def expected_returns(pnls_type, pnls_snapshot, pnl_dates_algo, pnl_weights, config_risk_scale):
def rand_weights(n):
    ''' Produces n random weights that sum to 1 '''
    k = np.random.rand(n)
    return k / sum(k)

def random_portfolio(returns):
    ''' 
    Returns the mean and standard deviation of returns for a random portfolio
    '''

    p = np.asmatrix(np.mean(returns, axis=1))
    w = np.asmatrix(rand_weights(returns.shape[0]))
    C = np.asmatrix(np.cov(returns))

    mu = w * p.T
    sigma = np.sqrt(w * C * w.T)

    # This recursion reduces outliers to keep plots pretty
    if sigma > 2:
        return random_portfolio(returns)
    return mu, sigma

def sharpe_ratio(W):
    '''Returns the sharpe ratio of the portfolio with weights.'''
    MeanVW = np.dot(MeanV, W)
    CovMW = np.sqrt(np.dot(np.dot(W, CovM), W.T))
    return((-1) * MeanVW / CovMW)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-fl', dest = 'cl_fname', help = 'filename of file with list of configs to consider')
    parser.add_argument('-sd', dest = 'start_trade_date', help = 'start trade date for risk allocation', required = True)
    parser.add_argument('-ed', dest = 'end_trade_date', help = 'end trade date for risk allocation', required = True)
    #parser.add_argument('-epa', dest = 'ep_algo', help = 'algo to construct expected pnl series data')
    parser.add_argument('-lb', dest = 'min_allocation', required = True, type = int, help = 'minimum allocation per config')
    parser.add_argument('-ub', dest = 'max_allocation', required = True, type = int, help = 'maximum allocation per config')
    parser.add_argument('-s', dest = 'scale', required = True, type = int, help = 'scaling factor')

    # read configs
    args = parser.parse_args()
    config_list = None
    unit_risk_scale = None
    (config_list, unit_risk_scale) = read_configs(args.cl_fname, None)
    configid_list = fetch_configs_id(config_list)
    # real pnls / out of sample sim results
    # eod pnls / intraday pnls
    # weights across days / weights across similar expected days
    # it is also important to do this study on least possible risk a strategy can be run at, which we call unit_risk
    pnls_type = 2
    pnls_snapshot = 2
    pnls_expected_series_algo = [1, 40, 100, 250, 0.5, 0.35, 0.15]

    scale = args.scale

    if pnls_type == 1:
        print("using real pnls")
        if pnls_snapshot == 1:
            print("using sample pnls")
        else:
            print("eod pnl snapshot")
    elif pnls_type == 2:
        if pnls_snapshot == 1:
            print("using sample pnls")
        else:
            if pnls_expected_series_algo[0] == 1:
                start_trade_date = datetime.strptime(str(args.start_trade_date), "%Y%m%d")
                end_trade_date = datetime.strptime(str(args.end_trade_date), "%Y%m%d")
                trade_date = start_trade_date
                while (trade_date.weekday() >= 5):
                    trade_date = trade_date + BDay(1)

                while (trade_date <= end_trade_date):
                    end_date = trade_date
                    end_date = end_date - BDay(1)
                    start_date = (end_date - BDay(pnls_expected_series_algo[3])).strftime("%Y%m%d")
                    end_date = end_date.strftime("%Y%m%d")

                    eod_pnl_df, daily_min_pnl_df, daily_dd_df = fetch_config_eod_stats(configid_list, start_date, end_date)
                    # dates * strats
                    eod_pnl_df.dropna(axis=0, how='any', inplace=True)
                    daily_min_pnl_df.dropna(axis=0, how='any', inplace=True)
                    daily_dd_df.dropna(axis=0, how='any', inplace=True)
                
                    NDays = len(eod_pnl_df)
                    NStrats = len(eod_pnl_df.T)

                    # day weighted ( expectations logic )
                    Wghts = np.array([pnls_expected_series_algo[4], pnls_expected_series_algo[5], pnls_expected_series_algo[6]])
                    Wghts = np.repeat(Wghts, [pnls_expected_series_algo[1], pnls_expected_series_algo[2], NDays - (pnls_expected_series_algo[1] + pnls_expected_series_algo[2])])              
                    w_eod_pnl_df = (eod_pnl_df.T) * Wghts

                    # risk normalized
                    NormFac = daily_dd_df.quantile(0.95)
                    #NormFac = daily_min_pnl_df.abs().quantile(0.95)
                    n_w_eod_pnl_df = w_eod_pnl_df.div(NormFac, axis=0)
                    n_w_eod_pnl_df.dropna(axis=1, how='any', inplace=True)

                    n_eod_pnl_df = eod_pnl_df.T.div(NormFac, axis=0)
                    n_eod_pnl_df.dropna(axis=1, how='any', inplace=True)

                    CovM = np.cov(n_eod_pnl_df, aweights=Wghts)
                    VarV = np.var(n_w_eod_pnl_df, axis=1, ddof=1)
                    MeanV = np.mean(n_w_eod_pnl_df, axis=1)
                    #print(MeanV)

                    # how can we restrict to integers
                    # a) upper bounds and lower bounds, scale uts to 5 everywhere and compute results, scaling down could give a better estimate
                    # b) 
                    bnds = ((args.min_allocation/scale, args.max_allocation/scale ),) * NStrats
                    #bnds = tuple((args.min_allocation*x/scale, args.max_allocation*x/scale) for x in NormFac.values)
                    # we could minimize the dd/pnl numbers ?
                    # we could maximize the mean for fixed risk

                    OptSoln = minimize(fun=sharpe_ratio, x0=VarV.T, method='L-BFGS-B', bounds=bnds)
                    print(",".join([str(x) for x in OptSoln.x]))        
                    # best portfolio
                    MeanVW = np.dot(np.mean(w_eod_pnl_df, axis=1), OptSoln.x) * NDays / np.sum(Wghts)
                    CovMW = np.sqrt(np.dot(np.dot(OptSoln.x, np.cov(w_eod_pnl_df)), OptSoln.x.T)) * NDays / np.sum(Wghts)

                    # not the right way, instead we simulate the portoflio numbers for n days and then use 0.95P
                    DDVW = np.dot(NormFac, OptSoln.x)
                    print("Date: " + trade_date.strftime("%Y%m%d"))
                    print("SimMean: " + str(MeanVW))
                    print("SimSharpe: " + str(MeanVW / CovMW))
                    print("SimMinPnl: " + str(DDVW))
                    #print(10000 * OptSoln.x)
                    risk_uts = [int(round(x)) for x in list(scale*OptSoln.x)]
                    print("INST_UTS: " + " ".join(str(x) for x in risk_uts))
                    r_eod_pnl_df, r_daily_min_pnl_df, r_daily_dd_df = fetch_config_eod_stats(configid_list, trade_date.strftime("%Y%m%d"), trade_date.strftime("%Y%m%d"))
                    const_pnl = (r_eod_pnl_df * risk_uts)/scale
                    port_pnl = const_pnl.sum(axis=1)
                    #print("CONST_PNL: " + " ".join(str(x) for x in const_pnl.values.tolist()))
                    if len(const_pnl.values) > 0:
                        print("RealConstPnl: " + " ".join(str(x) for x in (const_pnl.values[0])))
                        print("RealPortPnl: " + str(port_pnl.values[0]))
                    print("\n")
                    trade_date = trade_date + BDay(1)

            elif pnls_expected_series_algo[0] == 2:
                print("using similar days historical days pnls")
            else:
                print("not sure what to do")
    else:
        print("not sure what to do")


