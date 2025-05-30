#!/usr/bin/env python

"""
This script will output the performance of a pool on higher or lower value of a feature 
 USAGE : SHC <start_time end_time for pnl> start_date end_date <Feature AvgPrice/STDEV> <start_time end_time for feature>\
 [use prev day for feature = 0] [feature_shc = SHC] [split_percentile = 0.7]
"""


import subprocess
import pandas as pd
import numpy as np
import sys
import os
import getpass
import time

sys.path.append(os.path.expanduser('~/basetrade/'))  # relying on PYTHONPATH

from walkforward.utils import date_utils
from walkforward.definitions import execs
from walkforward.utils.run_exec import exec_function
from PyScripts.generate_dates import get_traindates


def load_date_value_map(shc, f_start_time, f_end_time, start_date, end_date, feature, date_value, available_dates):
    # fill the map of date_value by calling get_avg_samples.pl for each day and keep a count of 0 value in avg-samples
    count = 0
    date = int(end_date)
    while date >= start_date:
        if date not in available_dates:
            date = date_utils.calc_prev_week_day(date)
            continue
        if feature == "AvgPrice":
            avg_sample_cmd = [execs.execs().avg_samples, shc, str(date), "1", f_start_time, f_end_time, "2", feature]
            avg_sample_cmd = " ".join(avg_sample_cmd)
            data = exec_function(avg_sample_cmd)[0].strip()  # run get_avg_samples
            if len(data.split(" ")) < 3:  # this would happen if no result is printed
                date = date_utils.calc_prev_week_day(date)
                continue
            open_ = data.split(" ")[2].split("\n")[0]   # get avg of first 15 minutes
            close_ = data.split(" ")[-1].split("\n")[0]  # get avg of last 15 mins
            date_value[date] = (round((float(close_) - float(open_)), 4))
            if float(open_) == 0 or float(close_) == 0:
                count += 1
        else:
            avg_sample_cmd = [execs.execs().avg_samples, shc, str(date), "1", f_start_time, f_end_time, "0", feature]
            avg_sample_cmd = " ".join(avg_sample_cmd)
            data = exec_function(avg_sample_cmd)[0].strip()
            if len(data.split(" ")) < 3:
                date = date_utils.calc_prev_week_day(date)
                continue
            data = data.split(" ")[2].split("\n")[0]
            date_value[date] = (data)
            if float(data) == 0:
                count += 1
        date = date_utils.calc_prev_week_day(date)
    # this is printed so that if there are lot of zeros in the sample the user knows about it
    print("There were " + str(count) + " zeros in avg data \n")


def get_pool_results(shc, p_start_time, p_end_time, start_date, end_date, skip_dates_file, specific_dates_file):
    # Get the PnL data for the pool using summarize_strategy
    summarize_pnl_dir = execs.execs().modelling + "/wf_strats/" + shc + "/" + p_start_time + "-" + p_end_time + "/"
    summarize_pnl_command = [execs.execs().summarize_strategy, shc, summarize_pnl_dir, "DB", str(
        start_date), end_date, skip_dates_file, "kCNAPnlSharpe", "0", specific_dates_file, "0"]
    summarize_pnl_command = " ".join(summarize_pnl_command)
    pnl_data = exec_function(summarize_pnl_command)[0].strip()
    return pnl_data


if __name__ == "__main__":
    USAGE = "USAGE : SHC <start_time end_time for pnl> start_date end_date <Feature> <start_time end_time for feature>\
     <split_percentile> [HIGH/LOW=HIGH] [use prev day for feature = 0] [feature_shc = SHC] "

    if(len(sys.argv) < 9):
        print(USAGE + "\n")
        sys.exit(1)

    shc = sys.argv[1]
    p_start_time = sys.argv[2]
    p_end_time = sys.argv[3]
    start_date = int(sys.argv[4])
    end_date = sys.argv[5]
    Feature = sys.argv[6]
    f_start_time = sys.argv[7]
    f_end_time = sys.argv[8]
    split_percentile = float(sys.argv[9])

    if split_percentile < 0 or split_percentile > 1:
        print("Split Percentile should be between zero and one \n")
        sys.exit()

    split_percentile *= 100

    high_low = "HIGH"
    date_prev = 0
    feature_shc = shc

    if len(sys.argv) > 10:
        high_low = sys.argv[10]

    if len(sys.argv) > 11:
        date_prev = int(sys.argv[11])

    if len(sys.argv) > 12:
        feature_shc = sys.argv[12]

    work_dir = "/media/shared/ephemeral16/" + getpass.getuser() + "/feature_data/" + shc + "/"

    if not((os.path.exists(work_dir))):
        os.makedirs(work_dir)

    date_value_map = {}   # initialize map from date to feature value

    available_dates = list(map(int, get_traindates(min_date=start_date, max_date=end_date)))
    # load the date value map
    load_date_value_map(feature_shc, f_start_time, f_end_time, start_date, end_date, Feature, date_value_map, available_dates)
    # load the features to a numpy array
    features = np.array([float(val) for val in date_value_map.values()])
    # get the split value
    split_value = np.percentile(features, split_percentile)

    # create the file for low and high split values
    low_value_dates = work_dir + feature_shc + "_" + Feature + "_low_" + str(int(time.time() * 1000))
    high_value_dates = work_dir + feature_shc + "_" + Feature + "_high_" + str(int(time.time() * 1000))

    lf = open(low_value_dates, "w")
    hf = open(high_value_dates, "w")

    dates = np.sort(np.array([key for key in date_value_map.keys()]))
    print(dates)

    for i in range(len(dates) - 1):
        if float(date_value_map[dates[i]]) <= split_value:
            lf.write(str(dates[i + date_prev]) + "\n")
        else:
            hf.write(str(dates[i + date_prev]) + "\n")
    lf.close()
    hf.close()

    print("Split value is " + str(split_value) + "\n")
    print("Dates with lower split are in file " + low_value_dates +
          "\nDates with higher split are in file " + high_value_dates + "\n")

    if high_low == "HIGH":
        hf_data = get_pool_results(shc, p_start_time, p_end_time, start_date, end_date, "IF", high_value_dates)
        print("Summarized STRATEGY Results on high percentile date ---------------------\n" + hf_data + "\n")
    else:
        lf_data = get_pool_results(shc, p_start_time, p_end_time, start_date, end_date, "IF", low_value_dates)
        print("Summarized STRATEGY Results on low percentile date ---------------------\n" + lf_data + "\n")
