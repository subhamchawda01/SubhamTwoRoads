#!/usr/bin/env python
import pandas as pd
import numpy as np
import argparse
import subprocess

from walkforward.utils.search_exec_or_script import search_script

"""
some date wrapper functions from perl
"""


def get_list_of_dates_for_shortcode(shortcode, end_date, lookback):
    # first make decision about what look back parameter in perl call is
    cmd_ = search_script('get_list_of_dates_for_shortcode.pl') + " " + \
        shortcode + " " + str(end_date) + " " + str(lookback)
    out = subprocess.Popen(cmd_, shell=True, stdout=subprocess.PIPE)
    tmp = out.communicate()[0].decode('utf-8').strip().split(" ")
    LOD_output = np.array([int(x) for x in tmp])
    output = list(LOD_output)
    output.reverse()
    return output


def get_trading_days_for_shortcode(shortcode, num_days, end_date, lookback):
    # first make decision about what look back parameter in perl call is
    cmd_ = search_script('get_list_of_dates_for_shortcode.pl') + " " + shortcode + \
        " " + str(end_date) + " " + str(int(lookback) + num_days)
    out = subprocess.Popen(cmd_, shell=True, stdout=subprocess.PIPE)
    tmp = out.communicate()[0].decode('utf-8').strip().split(" ")
    LOD_output = np.array([int(x) for x in tmp])
    output = list(LOD_output)
    output.reverse()
    return output


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-S', dest='shortcode', help="SHC", type=str)
    parser.add_argument('-s', dest='start_date', help="start date", type=int, const=0, nargs='?')
    parser.add_argument('-e', dest='end_date', help="end date", type=int, const=0, nargs='?')
    parser.add_argument('-l', dest='lookback', help="lookback", type=int, const=0, nargs='?')

    args = parser.parse_args()
    print(get_trading_days_for_shortcode(args.shortcode, args.start_date, args.end_date, args.lookback))
