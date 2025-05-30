#!/usr/bin/env python
"""
Script to get pnl_series_correlation_with_pool
"""

import os
import sys
import pandas as pd
import numpy as np
from datetime import datetime

import shutil

sys.path.append(os.path.expanduser('~/basetrade/'))
from walkforward.definitions import execs
from pylib.pnl_modelling_utils.load_config_date_pnl_map import load_config_date_pnl_map
from walkforward.utils import date_utils
from walkforward.utils.run_exec import exec_function
import argparse
import pandas as pd


def load_strat_date_pnl_for_strat(results_dir, strat_name, start_date, end_date):
    """

    :param results_dir: 
    :param strat_name: 
    :param start_date: 
    :param end_date: 
    :return: 
    """
    summarize_pnl_cmd = execs.execs().summarize_local_results_dir_and_choose_by_algo + " kCNAPnlSharpe\
    4000 4000 -1 1 5000 100000 " + results_dir + " " + str(start_date) + " " + str(end_date)
    strate_date_pnl = {}
    pnls_temp = []

    PnL_data = exec_function(summarize_pnl_cmd)[0].strip()
    PnL_data = PnL_data.split("\n")
    for line in PnL_data:
        if line == "":
            continue
        words = line.split(" ")
        if words[0] == "STRATEGYFILEBASE":
            strat = str(words[1])
            if strat != strat_name:
                continue
            strate_date_pnl[strat] = {}
            continue
        if words[0] == "STATISTICS":
            continue
        if strat != strat_name:
            continue
        date_ = int(words[0])
        strate_date_pnl[strat][date_] = int(words[1])
        pnls_temp.append(int(words[1]))

    np_pnls_temp = np.array(pnls_temp)
    print("Strategy avg pnl : " + str(int(np_pnls_temp.mean())) +
          " and sharpe : " + str(round(np_pnls_temp.mean() / np_pnls_temp.std(), 2)))
    return strate_date_pnl


def get_correlation_and_sharpe_array(config_date_pnl_, strat_date_pnl_map_):
    """

    :param config_date_pnl_: 
    :param strat_date_pnl_map_: 
    :return: 
    """
    corrs = []
    combined_sharpes = []
    config_sharpes = []
    config_pnls = []
    strat_pnls = []
    strat_sharpes = []
    combined_pnls = []
    strategy_sharpe = []
    num_days = []
    max_corr = 0
    max_corr_config = ""
    config = list(strat_date_pnl_map_.keys())[0]
    for config2 in config_date_pnl_:

        pnl_strat = []
        pnl_config_pool = []
        for date in strat_date_pnl_map_[config]:
            if date in config_date_pnl_[config2] and date in strat_date_pnl_map_[config]:
                pnl_config_pool.append(int(config_date_pnl_[config2][date]))
                pnl_strat.append(int(strat_date_pnl_map_[config][date]))
        if len(pnl_strat) == 0 or len(pnl_config_pool) == 0:
            continue
        np_pnl_strat = np.array(pnl_strat)
        np_pnl_config_pool = np.array(pnl_config_pool)
        np_pnl_combined = np_pnl_strat + np_pnl_config_pool
        corr = np.corrcoef(pnl_strat, pnl_config_pool)[0][1]
        corrs.append(round(corr, 2))
        combined_sharpe = (np_pnl_combined.mean() / np_pnl_combined.std())
        combined_sharpes.append(round(combined_sharpe, 2))
        config_sharpes.append(round(np_pnl_config_pool.mean() / np_pnl_config_pool.std(), 2))
        strategy_sharpe.append(np_pnl_strat.mean() / np_pnl_strat.std())
        num_days.append(len(np_pnl_strat))
        config_pnls.append(int(np_pnl_config_pool.mean()))
        strat_pnls.append(int(np_pnl_strat.mean()))
        strat_sharpes.append(round(np_pnl_strat.mean() / np_pnl_strat.std(), 2))
        combined_pnls.append(int(np_pnl_combined.mean()))

    df = pd.DataFrame({'corrs': corrs, 'days': num_days, 'CombSharpe': combined_sharpes, 'Csharpe': config_sharpes,
                       'CombPnls': combined_pnls, 'Cpnls': config_pnls})
    cols = ['Cpnls', 'CombPnls', 'Csharpe', 'CombSharpe', 'corrs', 'days']
    df = df[cols]
    df = df.sort_values(['Csharpe'], ascending=False)
    max_corr = df["corrs"].max()
    top_five_corr_stats_ = (df.iloc[range(min(df.shape[0], 5))])
    print(df.to_string(index=False))
    return top_five_corr_stats_, max_corr


def get_top_five_corr_stats(shortcode, pool_times, start_date, end_date,
                            tag, strat_name, results_dir):
    """

    :param shortcode: 
    :param pool_times: 
    :param start_date: 
    :param end_date: 
    :param tag: 
    :param strat_name: 
    :param results_dir: 
    :return: 
    """
    config_date_pnl = load_config_date_pnl_map(shortcode, pool_times, start_date, end_date,
                                               tag)

    strat_date_pnl_map = load_strat_date_pnl_for_strat(results_dir, strat_name, start_date,
                                                       end_date)
    top_five_corr_stats, max_corr = get_correlation_and_sharpe_array(config_date_pnl, strat_date_pnl_map)
    return top_five_corr_stats, max_corr


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-shc', dest='shortcode', type=str, required=True)
    parser.add_argument('-pt', dest='pool_times', required=True)
    parser.add_argument('-sd', dest='start_date', required=True)
    parser.add_argument('-ed', dest='end_date', required=True)
    parser.add_argument('-t', dest='tag', help="Normal or Staged (N/S)", default="N", required=False)
    parser.add_argument('-res', dest='results_dir', required=True)
    parser.add_argument('-sname', dest='strat_name', help="Strat name to get the correlations", required=True)

    args = parser.parse_args()
    get_top_five_corr_stats(args.shortcode, args.pool_times, args.start_date, args.end_date, args.tag,
                            args.strat_name, args.results_dir)
