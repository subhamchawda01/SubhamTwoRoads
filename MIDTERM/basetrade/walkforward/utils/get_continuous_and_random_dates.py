#!/usr/bin/env python

"""

For a given shortcode and given trading_date, return mix of continuous and random days
 the continuous days would be from trading_date - num_continuous_days to tradingdate
 the random days would be from trading_date-num_continuous_days - look_back_random to trading_date - num_continuous_days
 
"""
import sys
import random
import subprocess

from walkforward.utils.date_utils import calc_prev_week_day
from walkforward.definitions.execs import execs


def get_continuous_and_random_dates(shortcode, last_date, lookback_continuous, lookback_random,
                                    num_random_days_to_choose, exclude_days):
    """
    Return the combined list of continuous and random dates from the given end date and date range
    :param shortcode: 
        product in question
    :param last_date:
        last trading days
    :param lookback_continuous: 
        number of continuous days we want, the end date would be last trading date
    :param lookback_random: 
        the set of total number of days we want to pick random dates from 
    :param num_random_days_to_choose:  
        total number of days to add randomly from larger set 
    :param exclude_days: 
        days to be excluded from lookback_radom days 
    :return:
     vector containing final mix of continuous and random dates
    """

    total_number_of_days = lookback_continuous + lookback_random
    continuous_start_date = calc_prev_week_day(last_date, lookback_continuous)

    # get the list of all dates
    dates_cmd = [execs().get_dates, shortcode, str(total_number_of_days), str(last_date)]
    process = subprocess.Popen(' '.join(dates_cmd), shell=True,
                               stderr=subprocess.PIPE,
                               stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')
    errcode = process.returncode
    if len(err) > 0:
        raise ValueError("Error in retrieving dates")

    list_of_all_days = list(map(int, out.split()))

    final_list_of_dates = []

    # collect the continuous days
    for date in list_of_all_days:
        if date >= continuous_start_date:
            final_list_of_dates.append(date)

    # remove from all dates
    for date in final_list_of_dates:
        list_of_all_days.remove(date)

    # remove the exclude days as well
    for date in exclude_days:
        if date in list_of_all_days:
            list_of_all_days.remove(date)

    if len(list_of_all_days) <= num_random_days_to_choose:
        print(sys.stderr, "Random days to choose from set is smaller than random_days_to_return, returning all..")

    num_random_days_to_choose = min(num_random_days_to_choose, len(list_of_all_days))

    frequency = int(len(list_of_all_days)/num_random_days_to_choose)

    print('LIST_OF_DAYS AND FREQUENCY', list_of_all_days, frequency)
    
    # sample required number of dates from remaining dates
    new_random_dates = list_of_all_days[::frequency]
    final_list_of_dates += new_random_dates

    return final_list_of_dates
