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
from RiskAllocation.fetch_config_performance_stats import *


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


def emean_sort_fun(a, b):
    if MeanV.iloc[a] > MeanV.iloc[b]:
        return 1
    elif MeanV.iloc[a] == MeanV.iloc[b]:
        return 0
    else:
        return -1

def evar_sort_fun(a, b):
    a_wghts = __w__
    b_wghts = __w__    
    a_wghts[a] += 1
    b_wghts[b] += 1
    a_sd = np.sqrt(np.dot(np.dot(a_wghts, CovM), a_wghts.T))
    b_sd = np.sqrt(np.dot(np.dot(b_wghts, CovM), b_wghts.T))
    a_wghts[a] -= 1
    b_wghts[b] -= 1
    if a_sd > b_sd:
        return 1
    elif a_sd == b_sd:
        return 0
    else:
        return -1
   
def cmp_to_key(mycmp):
    'Convert a cmp= function into a key= function'
    class K(object):
        def __init__(self, obj, *args):
            self.obj = obj
        def __lt__(self, other):
            return mycmp(self.obj, other.obj) < 0
        def __gt__(self, other):
            return mycmp(self.obj, other.obj) > 0
        def __eq__(self, other):
            return mycmp(self.obj, other.obj) == 0
        def __le__(self, other):
            return mycmp(self.obj, other.obj) <= 0  
        def __ge__(self, other):
            return mycmp(self.obj, other.obj) >= 0
        def __ne__(self, other):
            return mycmp(self.obj, other.obj) != 0
    return K

def round_off(awghts, algo=None):
    iwghts = np.floor(awghts)
    fwghts = awghts - iwghts
    idx = list(range(len(awghts)))
    if algo == "MeanSorted":
        algo_sorted_idx = (sorted(idx, key=cmp_to_key(emean_sort_fun)))
    elif algo == "VarSorted":
        algo_sorted_idx = (sorted(idx, key=cmp_to_key(evar_sort_fun)))
    else :
        algo_sorted_idx = np.argsort(fwghts)        
    #DeltaEM
    DeltaEM =  np.dot(MeanV, awghts) - np.dot(MeanV, iwghts)
    for t_idx in algo_sorted_idx:
        if(DeltaEM > 0 and MeanV.iloc[t_idx] > 0 and fwghts.iloc[t_idx] > 0):
            #print("Delta " + str(DeltaEM))
            #print(str(t_idx) + " " + str(fwghts.iloc[t_idx]))
            iwghts.iloc[t_idx] += 1
            #print(MeanV.iloc[t_idx])
            DeltaEM -= MeanV.iloc[t_idx]
    #VarEM
    return(iwghts)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-fl', dest = 'cl_fname', help = 'filename of file with list of configs to consider')
    parser.add_argument('-sd', dest = 'start_trade_date', help = 'start trade date for risk allocation', required = True)
    parser.add_argument('-ed', dest = 'end_trade_date', help = 'end trade date for risk allocation', required = True)
    #parser.add_argument('-epa', dest = 'ep_algo', help = 'algo to construct expected pnl series data')
    parser.add_argument('-lb', dest = 'min_allocation', required = True, type = int, help = 'minimum allocation per config')
    parser.add_argument('-ub', dest = 'max_allocation', required = True, type = int, help = 'maximum allocation per config')
    parser.add_argument('-rf', dest = 'ralgo', required = True, type = int, help = 'round off algo')
    parser.add_argument('-bl', dest = 'bl', required = True, type = int, help = 'bounds logic')
    parser.add_argument('-file_write', dest='file_write', default = 1, type = int, help = '0 if overwriting the portfolio is not required')

    # read configs
    args = parser.parse_args()
    config_list = None
    unit_risk_scale = None
    (config_list, unit_risk_scale) = read_configs(args.cl_fname, None)
    configs_info = fetch_configs_id(config_list)
    config_list = configs_info['cname']
    configid_list = configs_info['configid']
    # real pnls / out of sample sim results
    # eod pnls / intraday pnls
    # weights across days / weights across similar expected days
    # it is also important to do this study on least possible risk a strategy can be run at, which we call unit_risk
    pnls_type = 2
    pnls_snapshot = 2
    pnls_expected_series_algo = [1, 40, 100, 250, 0.5, 0.35, 0.15]

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

                    # day weighted and risk normalized
                    NormFac = daily_dd_df.quantile(0.95)
                    #NormFac = daily_min_pnl_df.abs().quantile(0.95)
                    n_w_eod_pnl_df = w_eod_pnl_df.div(NormFac, axis=0)
                    n_w_eod_pnl_df.dropna(axis=1, how='any', inplace=True)

                    # just risk normalized
                    n_eod_pnl_df = eod_pnl_df.T.div(NormFac, axis=0)
                    n_eod_pnl_df.dropna(axis=1, how='any', inplace=True)

                    CovM = np.cov(n_eod_pnl_df, aweights=Wghts.T) # note we are using weights as inputs
                    VarV = np.var(n_w_eod_pnl_df, axis=1, ddof=1)
                    MeanV = np.mean(n_w_eod_pnl_df, axis=1)
                    #print(MeanV)

                    #how can we restrict to integers
                    bnds = ((0, 0),) * NStrats                    
                    if args.bl == 1:
                        bnds = ((args.min_allocation, args.max_allocation),) * NStrats
                    else:
                        args.bl = 2; # set bl to 2 as we are using this in naming of output file
                        bnds = tuple((args.min_allocation * x / max(NormFac.values), args.max_allocation * x / max(NormFac.values)) for x in NormFac.values)

                    OptSoln = minimize(fun=sharpe_ratio, x0=VarV.T, method='L-BFGS-B', bounds=bnds)
                    # best portfolio
                    print("#Date: " + trade_date.strftime("%Y%m%d"))
                    print("#OptValue: " + str(-sharpe_ratio(OptSoln.x)))
                    print("#Weights: " + " ".join(str(x) for x in list(OptSoln.x)))

                    # rounding of weights W
                    # suppose we are targetting T as a target pnl and with given weights we have E pnl
                    # we just need to scale weights to W = T / E * W 
                    # we are sure about floor value weights, 
                    # a) remaining are scaled based [pnl_diff] - this_const_floor / [pnl_diff] - this_const_decimal
                    # b) remaining are scaled based on [pnl_diff] - this_const_floor / [pnl_diff] - this_const_decimal but considered after sorting

                    risk_uts = max(NormFac) * np.divide(OptSoln.x, NormFac)                    
                    print("#RiskUTS: " + " ".join(str(x) for x in list(risk_uts)))
                    __w__ = np.floor(risk_uts)
                    if args.ralgo == 1:
                        inst_uts = round_off(risk_uts, "MeanSorted")
                    elif args.ralgo == 2:
                        inst_uts = round_off(risk_uts)
                    else:
                        args.ralgo = 3; # set ralgo to 3 as we are using this in naming of output file
                        inst_uts = (risk_uts).round(0).astype(int)

                    print("#InstUTS: " + " ".join(str(x) for x in inst_uts))

                    r_eod_pnl_df, r_daily_min_pnl_df, r_daily_dd_df = fetch_config_eod_stats(configid_list, trade_date.strftime("%Y%m%d"), trade_date.strftime("%Y%m%d"))
                    eod_const_pnl = (r_eod_pnl_df * inst_uts)
                    eod_port_pnl = eod_const_pnl.sum(axis=1)
                    if len(eod_const_pnl.values) > 0:
                        print("#RealConstPnl: " + " ".join(str(x) for x in (eod_const_pnl.values[0])))
                        print("#RealPortPnl: " + str(eod_port_pnl.values[0]))


                    # pick_strats_config
                    r_portfolio = inst_uts[inst_uts > 0]
                    print("#NStrats: " + str(len(r_portfolio)))
                    r_maxdd_df = fetch_weighted_portfolio_daily_maxdd(r_portfolio, trade_date.strftime("%Y%m%d"))
                    print("#DailyDD: " + " ".join(str(x) for x in r_maxdd_df.T.values[0]))
                    if args.file_write ==1:
                        final_df = pd.concat([config_list, pd.Series(inst_uts.values), pd.Series(risk_uts.values).round(3), pd.Series(NormFac.values).round(0)], axis=1)
                        final_df.index = configid_list
                        final_df.columns = ['CfgName', 'InstUTS', 'RiskUTS', '95PDD']
                        final_df = final_df.loc[final_df['InstUTS'] > 0]
                        output_fname = "/spare/local/MeanRevertPort/DailyPort/" + "MRT_Portfolio_" + str(args.ralgo) + "_" + str(args.bl) + "_" + str(trade_date.strftime("%Y%m%d"))
                        #output_fname = "MRT_Portfolio_" + str(args.ralgo) + "_" + str(args.bl) + "_" + str(trade_date.strftime("%Y%m%d"))
                        final_df.to_csv(output_fname)
                    print("\n")
                    trade_date = trade_date + BDay(1)
            elif pnls_expected_series_algo[0] == 2:
                print("using similar days historical days pnls")
            else:
                print("not sure what to do")
    else:
        print("not sure what to do")
