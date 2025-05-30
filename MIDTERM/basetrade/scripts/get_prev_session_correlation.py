#!/usr/bin/env python

""" The script finds correlation between Features of the prev session with the PnL of a strategy and exploit it to use it in our strategies
For now it only expects AvgPrice or STDEV, for AvgPrice it takes the feature value as close - open (avg of last 15 mins - avg of first 15 mins) and for Stdev avg(open,close). 
start_time end_time for pnl should be start time end time of a pool
It outputs for each strategy the correlation of PnL with the Feature and session specified, correlation of Pnl with absolute value of Feature, correlation of Pnl with Feature on days with positive PnL and correlation of Pnl with Feature on days with negative PnL
script usage example 
scripts/get_prev_session_correlation.py JGBL_0 JST_1531 EST_700 20161201 20170501 STDEV JST_900 JST_1500"""


import subprocess
import pandas as pd
import numpy as np
import sys
import os

sys.path.append(os.path.expanduser('~/basetrade/'))  # relying on PYTHONPATH

from walkforward.utils import date_utils
from walkforward.definitions import execs
from walkforward.utils.run_exec import exec_function
#from walkforward.utils.search_exec_or_script import search_script, search_exec


def load_config_date_pnl_map(shc, p_start_time, p_end_time, start_date, end_date):

    # Get the PnL data for the pool using summarize_strategy
    summarize_pnl_dir = execs.execs().modelling + "/wf_strats/" + shc + "/" + p_start_time + "-" + p_end_time + "/"
    summarize_pnl_command = [execs.execs().summarize_strategy, shc, summarize_pnl_dir, "DB",
                             str(start_date), end_date, "IF", "kCNAPnlSharpe", "0", "IF", "0"]
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
            config_date_pnl[config] = {}
            continue
        if words[0] == "STATISTICS":
            continue
        date_ = int(words[0])
        config_date_pnl[config][date_] = words[1]


def load_date_value_map(shc, f_start_time, f_end_time, start_date, end_date):
    # fill the map of date_value by calling get_avg_samples.pl for each day and keep a count of 0 value in avg-samples
    count = 0
    date = date_utils.calc_prev_week_day(end_date)
    while date >= start_date:
        if Feature == "AvgPrice":
            avg_sample_cmd = [execs.execs().avg_samples, shc, str(date), "1", f_start_time, f_end_time, "2", Feature]
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
        elif Feature == "STDEV":
            avg_sample_cmd = [execs.execs().avg_samples, shc, str(date), "1", f_start_time, f_end_time, "0", Feature]
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


""" Get correlations for each config 
For that it creates a list of PnL and a list of feature Values
List of PnL and list of features Values for Positive PnL days 
List of PnL and list of features Values for Negative PnL days 
and prints the correlations for each config"""


def print_the_correlations(config_date_pnl, date_value):
    # print the appropriate header
    if Feature == "AvgPrice":
        print("STRATEGYFILEBASE Config Correlation_value_with_pnl , correlation_abs(value)_with_pnl, correlation_value_with_pnl(positive days), correlation_value_with_pnl(negative days)")
    elif Feature == "STDEV":
        print("STRATEGYFILEBASE Config Correlation_value_with_pnl, correlation_value_with_pnl(positive days), correlation_value_with_pnl(negative days)")

    for config in list(config_date_pnl.keys()):
        PnL = []
        values_ = []
        positive_pnl = []
        positive_values = []
        negative_pnl = []
        negative_values = []
        for dt in config_date_pnl[config]:
            try:
                values_.append(float(date_value[dt]))
                PnL.append(float(config_date_pnl[config][dt]))
                if float(config_date_pnl[config][dt]) > 0:
                    positive_values.append(float(date_value[dt]))
                    positive_pnl.append(float(config_date_pnl[config][dt]))
                else:
                    negative_values.append(float(date_value[dt]))
                    negative_pnl.append(float(config_date_pnl[config][dt]))
            except:
                pass
        if date_prev:
            PnL = PnL[1:]
            values_ = values_[:len(values_) - 1]
            positive_pnl = positive_pnl[1:]
            positive_values = positive_values[:len(positive_values) - 1]
            negative_pnl = negative_pnl[1:]
            negative_values = negative_values[:len(negative_values) - 1]
        if Feature == "AvgPrice":
            print("STRATEGYFILEBASE " + config + " :", np.corrcoef(PnL, values_)[0][1], np.corrcoef(PnL, [abs(val) for val in values_])[
                  0][1], np.corrcoef(positive_pnl, positive_values)[0][1], np.corrcoef(negative_pnl, negative_values)[0][1], "\n")
        elif Feature == "STDEV":
            print("STRATEGYFILEBASE " + config + " :", np.corrcoef(PnL, values_)
                  [0][1], np.corrcoef(positive_pnl, positive_values)[0][1], np.corrcoef(negative_pnl, negative_values)[0][1], "\n")


USAGE = "USAGE : SHC <start_time end_time for pnl> start_date end_date <Feature AvgPrice/STDEV> <start_time end_time for feature> [use prev day for feature = 0] [feature_shc = SHC]"
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
date_prev = 0

if len(sys.argv) > 9:
    date_prev = sys.argv[9]

feature_shc = shc
if len(sys.argv) > 10:
    feature_shc = sys.argv[10]

if Feature != "AvgPrice" and Feature != "STDEV":
    print("Expected AvgPrice or STDEV got " + Feature + "\n")
    print(USAGE + "\n")
    sys.exit(1)


date_value = {}  # map from date to feature value
config_date_pnl = {}  # map from config to map of date to pnl for the config

load_config_date_pnl_map(shc, p_start_time, p_end_time, start_date, end_date)

load_date_value_map(feature_shc, f_start_time, f_end_time, start_date, end_date)

print_the_correlations(config_date_pnl, date_value)
