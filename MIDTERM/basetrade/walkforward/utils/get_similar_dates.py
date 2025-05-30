#!/usr/bin/env python

# Get dates similar to current trigger STDEV/VOL/L1SZ/TREND
# \file walkforward/build_model.py


import os
import subprocess
import numpy
import sys

sys.path.append(os.path.expanduser('~/basetrade/'))
from walkforward.definitions import execs
from walkforward.utils.date_utils import calc_prev_week_day
# SIMILAR-STDEV-NA-20-0.1
# SIMILAR-CORR-NKM_0-20-0.5


def get_similar_dates(shortcode, num_days, date, start_time, end_time, model_for_date):
    '''
    Output the list of dates which are similar to current trigger date in terms of a variable (could be STDEV, VOL, CORR etc. as mentioned in the list in check_ddays_sting function).
    shortcode: str
            Product shortcode
    num_days: str
            DDAYS_STRING as mentioned in the config. The format of num_days is SIMILAR-STDEV-NA-20-0.1 if the variable is not CORR and SIMILAR-CORR-NKM_0-20-0.5 if the variable is CORR. The last two numbers represent lookback days and threshold respectively
    date: str
            Previous week date than model_for_date
    start_time: str
            The start time of the trading session
    end_time: str
            The end time of the trading session
    model_for_date:
            Trading date as passed in the run_compute_strat_for_config_and_date.py

    returns:
            A list of dates which are similar to the trigger date, based on the variable
    '''
    flag = check_ddays_string(num_days)
    if (flag == 1):
        return get_dates(shortcode, num_days, date, start_time, end_time, model_for_date)
    else:
        print("invalid argument to get_similar_dates.py, provide string as 'SIMILAR-VARIABLE-SHORTCODE-DAYS-THRESHOLD' where SHORTCODE is NA except when VARIABLE is CORR")
        sys.exit(0)


def check_ddays_string(num_days):
    '''
    Checks whether num_days (or DDAYS_STRING as present in the config) is valid or not.
    returns:
            flag=1 if the string is valid, -1 otherwise
    '''
    variable_list = ["VOL", "STDEV", "L1SZ", "L1EVPerSec", "TREND", "TrendStdev", "ORDSZ",
                     "TRADES", "SSTREND", "TOR", "BidAskSpread", "AvgPrice", "AvgPriceImpliedVol", "CORR"]
    if (num_days.split("-")[1] in variable_list) and (int(num_days.split("-")[3]) >= 0)\
            and (float(num_days.split("-")[4]) >= 0) and (len(num_days.split("-")) == 5):
        if (num_days.split("-")[1] == "CORR") & (num_days.split("-")[2] == "NA"):
            return -1
        else:
            return 1
    else:
        return -1


def get_dates(shortcode, num_days, date, start_time, end_time, model_for_date):
    '''
    returns:
            A list of dates
    '''
    target = get_curr_value(shortcode, model_for_date,
                            start_time, end_time, num_days)
    threshold = float(num_days.split("-")[4])
    curr_date = date
    total_days = int(num_days.split("-")[3])
    output = []
    while total_days > 0:
        prev_val = get_curr_value(
            shortcode, curr_date, start_time, end_time, num_days)
        if (prev_val <= (1 + threshold) * target) and (prev_val >= (1 - threshold) * target):
            output.append(str(curr_date))
            total_days = total_days - 1
        curr_date = calc_prev_week_day(curr_date, 1)
    return output


def get_curr_value(shortcode, model_for_date, start_time, end_time, num_days):
    '''
    A wrapper function to call the exec of ~/basetrade/scripts/get_avg_samples.pl
    returns:
            Daily average value of the VARIABLE for the given shortcode
    '''
    if num_days.split("-")[1] != "CORR":
        trigger_date = [execs.execs().avg_samples, shortcode, model_for_date,
                        1, start_time, end_time, 0, num_days.split("-")[1]]
    else:
        trigger_date = [execs.execs().avg_samples, shortcode, model_for_date, 1,
                        start_time, end_time, 0, num_days.split("-")[1], num_days.split("-")[2]]
    process = subprocess.Popen(" ".join(str(i) for i in trigger_date),
                               shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')
    errcode = process.returncode
    if len(err) > 0:
        raise ValueError("Error in retrieving dates")
    out = float(str(out).split(":")[1].strip("\\n'"))
    return out
