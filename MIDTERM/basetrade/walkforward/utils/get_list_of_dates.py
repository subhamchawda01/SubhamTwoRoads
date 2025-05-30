#!/usr/bin/env python

import sys
import random
import subprocess

from walkforward.utils.date_utils import calc_prev_week_day
from walkforward.definitions.execs import execs


def get_list_of_dates(shortcode, lookback_days, last_date):
    """

    :param shortcode: 
    :param lookback_days: 
    :param last_date: 
    :return: list_of_all_days
    """
    # get the list of all dates
    dates_cmd = [execs().get_dates, shortcode, str(lookback_days), str(last_date)]
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

    list_of_all_dates = list(map(int, out.split()))
    return list_of_all_dates
