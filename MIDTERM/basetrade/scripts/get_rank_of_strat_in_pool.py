#!/usr/bin/env python
"""
Script to get rank of strategy in pool
"""

import os
import sys
import pandas as pd
import numpy as np
from datetime import datetime
import argparse

sys.path.append(os.path.expanduser('~/basetrade/'))
from walkforward.definitions import execs
from walkforward.utils.run_exec import exec_function


def load_config_score_map(shc, summarize_pnl_pool, results_dir, start_date, end_date, pool_tag, strat_name,
                          dates_file="IF", sort_algo="kCNAPnlSharpeAverage"):
    config_score_map = {}
    summarize_pnl_command = [execs.execs().summarize_strategy, shc, summarize_pnl_pool, results_dir, str(start_date),
                             str(end_date), "IF", sort_algo, "0", dates_file, "1", pool_tag]
    summarize_pnl_command = " ".join(summarize_pnl_command)

    PnL_data = exec_function(summarize_pnl_command)[0].strip()

    # fill the map of config_date_pnl from output of summarize strategy results
    PnL_data = PnL_data.split("\n")
    for line in PnL_data:
        if line == "":
            continue
        words = line.split(" ")
        if words[0] == "STRATEGYFILEBASE":
            config = words[1]
            if strat_name == "ALL" or config in strat_name:
                score = float(words[-1])
                config_score_map[config] = score

    return config_score_map


def get_rank_of_strat_in_pool(shortcode, pool_times, start_date, end_date, tag, strat_name, strats_dir, results_dir,
                              dates_file_="IF", sort_algo="kCNAPnlSharpeAverage"):
    """

    :param shortcode: 
    :param pool_times: 
    :param start_date: 
    :param end_date: 
    :param tag: 
    :param strat_name: 
    :param strats_dir: 
    :param results_dir: 
    :param dates_file_: 
    :param sort_algo: 
    :return: 
    """
    config_to_score_map = load_config_score_map(shortcode, pool_times, "DB", start_date, end_date, tag, "ALL",
                                                dates_file_, sort_algo)
    strat_to_score_map = load_config_score_map(shortcode, strats_dir, results_dir, start_date, end_date, "",
                                               strat_name, dates_file_, sort_algo)
    strat_score = strat_to_score_map[strat_name]
    strat_rank = 1
    num_pool_strats = 0
    for config in config_to_score_map:
        num_pool_strats += 1
        if config_to_score_map[config] >= strat_score:
            strat_rank += 1

    return strat_rank, num_pool_strats


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-shc', dest='shortcode', type=str, required=True)
    parser.add_argument('-pt', dest='pool_times', required=True)
    parser.add_argument('-sd', dest='start_date', required=True)
    parser.add_argument('-ed', dest='end_date', required=True)
    parser.add_argument('-t', dest='tag', help="Normal or Staged (N/S)", default="N", required=False)
    parser.add_argument('-sdir', dest='strats_dir', required=True)
    parser.add_argument('-res', dest='results_dir', required=True)
    parser.add_argument('-sname', dest='strat_name', help="Strat name to get the correlations", required=True)
    parser.add_argument('-dfile', dest='dates_file', help="file with the list of dates", default="IF", required=False)
    parser.add_argument('-sa', dest='sort_algo', help="sort_algo", default="kCNAPnlSharpeAverage", required=False)

    args = parser.parse_args()
    strategy_rank, num_pool_strats = get_rank_of_strat_in_pool(args.shortcode, args.pool_times, args.start_date, args.end_date, args.tag,
                                              args.strat_name, args.strats_dir, args.results_dir, args.dates_file, args.sort_algo)

    print(strategy_rank)
