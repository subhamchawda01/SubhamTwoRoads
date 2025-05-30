#!/usr/bin/env python
"""
Script to get negative dates of a pool
"""
import os
import sys
import argparse
import numpy as np

sys.path.append(os.path.expanduser('~/basetrade/'))
from walkforward.definitions import execs
from walkforward.utils.run_exec import exec_function
from PyScripts.generate_dates import get_traindates


def load_date_pnl_map(shc, summarize_pnl_pool, start_date, end_date, list_of_strats, available_dates):
    """

    :param shc: 
    :param summarize_pnl_pool: 
    :param start_date: 
    :param end_date: 
    :param list_of_strats: 
    :param date_pnl: 
    :return: 
    """
    # Get the PnL data for the pool using summarize_strategy
    summarize_pnl_command = [execs.execs().summarize_strategy, shc, summarize_pnl_pool, "DB",
                             str(start_date), str(end_date), "IF", "kCNAPnlSharpe", "0", "IF", "0"]
    summarize_pnl_command = " ".join(summarize_pnl_command)
    PnL_data = exec_function(summarize_pnl_command)[0].strip()

    # fill the map of date_pnl from output of summarize strategy results
    PnL_data = PnL_data.split("\n")
    date_pnl = {}
    for line in PnL_data:
        if line == "":
            continue
        words = line.split(" ")
        if words[0] == "STRATEGYFILEBASE":
            config = words[1]
            continue

        if (config not in list_of_strats and list_of_strats != "ALL") or words[0] == "STATISTICS":
            continue

        date_ = int(words[0])
        if date_ not in available_dates:
            continue
        if date_ in date_pnl.keys():
            date_pnl[date_] += int(words[1])
        else:
            date_pnl[date_] = int(words[1])

    return date_pnl


def get_n_negative_dates_of_pool(shortcode, pool_timings, start_date, end_date, list_of_strats, num_days, available_dates):
    """

    :param shortcode: 
    :param pool_timings: 
    :param start_date: 
    :param end_date: 
    :param list_of_strats: 
    :param num_days: 
    :return: 
    """

    date_pnl_map = load_date_pnl_map(shortcode, pool_timings, start_date, end_date, list_of_strats, available_dates)
    date_pnl_vals = np.array(list(date_pnl_map.values()))
    num_of_dates = len(date_pnl_vals)
    percentile = min(float(num_days) / num_of_dates * 100, 100)
    print(percentile)
    loss_threshold = np.percentile(date_pnl_vals, percentile)

    dates_to_return = []
    for date in date_pnl_map:
        if date_pnl_map[date] < loss_threshold:
            # print(date)
            dates_to_return.append(date)

    return dates_to_return


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-shc', dest='shortcode', help="shortcode to get negative dates", type=str, required=True)
    parser.add_argument('-pt', dest='pool_timings', help='start_time-end_time of the pool', type=str, required=True)
    parser.add_argument('-strats', dest='list_of_strats', help='start_time-end_time of the pool', type=str, default="ALL",
                        required=False)
    parser.add_argument('-sd', dest='start_date', required=True)
    parser.add_argument('-ed', dest='end_date', required=True)
    parser.add_argument('-days', dest='num_days', help="num of days you want as output", required=True)
    args = parser.parse_args()
    available_dates = list(map(int, get_traindates(min_date=args.start_date, max_date=args.end_date)))
    n_negative_days = get_n_negative_dates_of_pool(
        args.shortcode, args.pool_timings, args.start_date, args.end_date, args.list_of_strats, args.num_days, available_dates)
    print(n_negative_days)
