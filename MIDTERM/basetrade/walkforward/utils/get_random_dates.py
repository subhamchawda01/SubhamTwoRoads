#!/usr/bin/env python

"""

For a given shortcode and given trading_date, return a list of random dates within the lookback days

"""

import sys
import random
import subprocess

from walkforward.utils.date_utils import calc_prev_week_day
from walkforward.utils.get_list_of_dates import get_list_of_dates
from walkforward.definitions.execs import execs


def get_random_dates(shortcode, last_date, lookback_days,
                     num_random_days_to_choose, exclude_days=[]):
    """

    :param shortcode: 
    :param last_date: 
    :param lookback_days: 
    :param num_random_days_to_choose: 
    :param exclude_days: 
    :return: 
    """

    list_of_all_days = get_list_of_dates(shortcode, lookback_days, last_date)

    # remove the exclude days as well
    for date in exclude_days:
        if date in list_of_all_days:
            list_of_all_days.remove(date)

    if len(list_of_all_days) <= num_random_days_to_choose:
        print(sys.stderr, "Random days to choose from set is smaller than random_days_to_return, returning all..")

    num_random_days_to_choose = min(num_random_days_to_choose, len(list_of_all_days))

    random_indices = random.sample(range(len(list_of_all_days)), num_random_days_to_choose)

    final_list_of_dates = [list_of_all_days[i] for i in random_indices]

    return final_list_of_dates
