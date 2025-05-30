#!/usr/bin/env python

"""

Script that was written to help find optimal date intervals for run simulation calls.

Problem statement:
Context : in a type 4 config, we need to choose model and param for the next day based on past performance. For this we need to call run_simulations which
 is a time intensive process. For a given sequence of dates, model and param for a certain subset might exist. So, problem statement is:
Given a start date and lookback period and end_date, generate list of start and end dates for which run_simulations should be called.
Trivial case, when no model param is present, return (start-lookback, end_date)
Other edge case when model param for all days between start and end date is present, return None
Note that in output if any two sets of dates have overlap, these should be reduced to one tuple of dates.


returns:
--------
list of :
list of date tuples for which run_simulations should be called, list of dates when model/param is not present


Pseudo Code:
------------

1. First get list of dates for shortcode (start_end-lookback, end_date)
2. If model param does not exist for any of these dates, return the (start_end-lookback, end_date)
3. If model param exists for some dates, step through date list, at each step:
    A) if model/param not present, record appropriate date interval for run_simulations and do:
        a) take union of this date interval with previous interval (previous interval initialized to none)
        b) if union consists of two non overlapping date intervals, push first interval into set for final date intervals
        c) if union consists of one date interval, record this as previous interval
        d) Move to next date
    if model/param present, move to next date
4. Report final date intervals as the list of date tuples for run_simulations

This may not be the most efficient algo, but its correct and simple to reason about.
"""

import pandas as pd
import numpy as np
from functools import partial
import os
import sys
import argparse
import pdb


sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.wf_db_utils import get_trading_days_for_shortcode

from walkforward.wf_db_utils import fetch_strat_from_config_and_date

from walkforward.wf_db_utils import fetch_config_details


def is_strat_not_there(configname, tradingdate):
    """ check if model and param for a type 4 config are there in DB or not?. Returns true or false """

    (shortcode, execlogic, modelfilename, paramfilename, start_time, end_time, strat_type, event_token, query_id) \
        = fetch_strat_from_config_and_date.fetch_strat_from_config_and_date(configname, tradingdate, 1)
    # print tradingdate, modelfilename
    if modelfilename == "INVALID" or paramfilename == 'INVALID':
        return True
    else:
        return False


def unique(a):
    """ return the list with duplicate elements removed """
    return sorted(list(set(a)))


def intersect(a, b):
    """ return the intersection of two lists """
    return sorted(list(set(a) & set(b)))


def union(a, b):
    """ return the union of two lists """
    return sorted(list(set(a) | set(b)))


def get_optimized_days_for_which_model_param_is_missing(config, configname, num_days, end_date, lookback):
    """ Chief function that implements the algo as specified on top - run through dates, take unions and finally report date tuples. As a bonus report
    list of dates that dont have model/param
    """

    # the set of dates for which we check if model param exists
    principal_dates = get_trading_days_for_shortcode.get_list_of_dates_for_shortcode(config.shortcode, end_date,
                                                                                     num_days)
    principal_dates_array = np.array(principal_dates)
    if len(principal_dates) == 0:
        return [[]], []

    start_date = principal_dates[0]
    strat_not_present_on_date = np.array([is_strat_not_there(configname, date) for date in principal_dates])
    stratpresentondate = ~(strat_not_present_on_date)
    dates_when_strat_not_present = principal_dates_array[strat_not_present_on_date]
    dates_when_strat_not_present_list = list(dates_when_strat_not_present)

    #: Edge Case 1, if all days between start and end have model\param, return empty set
    #: Edge Case 2, if no days between start and end have model\param, return all the dates
    #: if the above cases are false, do the complex logic of finding the intervals

    if len(dates_when_strat_not_present) == 0:
        return [[]], dates_when_strat_not_present_list
    elif len(dates_when_strat_not_present) == len(principal_dates):
        begin_simulations_tmp = get_trading_days_for_shortcode.get_list_of_dates_for_shortcode(config.shortcode,
                                                                                               start_date, lookback)
        begin_simulations_date = begin_simulations_tmp[0]
        return [[begin_simulations_date, end_date]], dates_when_strat_not_present_list
    else:
        # step through dates where model param is not there, create window, take unions going forward, if no overlap start new interval

        previous_window_of_dates = [dates_when_strat_not_present[0]]
        final_date_windows = []
        for date in dates_when_strat_not_present:

            this_window_of_dates = get_trading_days_for_shortcode.get_list_of_dates_for_shortcode(
                config.shortcode, date, lookback)

            # this is where we take union, and also check of overlap if empty using intersect window

            current_relevant_window = union(this_window_of_dates, previous_window_of_dates)
            intersect_window = intersect(this_window_of_dates, previous_window_of_dates)

            # this means the continuity of dates has broken and new date pair has started

            if len(intersect_window) == 0:
                print("inserting into date pairs...")
                print([previous_window_of_dates[0], previous_window_of_dates[-1]])
                final_date_windows.append([previous_window_of_dates[0], previous_window_of_dates[-1]])
                previous_window_of_dates = [date]
            else:
                previous_window_of_dates = current_relevant_window
            if date == dates_when_strat_not_present[-1]:
                print("inserting into date pairs...final")
                final_date_windows.append([previous_window_of_dates[0], previous_window_of_dates[-1]])
        return final_date_windows, dates_when_strat_not_present_list


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument('-c', dest='config', help="", type=str)
    parser.add_argument('-C', dest='configname', help="", type=str)
    parser.add_argument('-e', dest='end_date', help="", type=int)
    parser.add_argument('-n', dest='numdays', help="", type=int)
    parser.add_argument('-l', dest='lookback', help="", type=int)

    args = parser.parse_args()

    config = fetch_config_details.fetch_config_details(args.configname)

    print((get_optimized_days_for_which_model_param_is_missing(config, args.configname, args.numdays, args.end_date,
                                                               args.lookback)))
