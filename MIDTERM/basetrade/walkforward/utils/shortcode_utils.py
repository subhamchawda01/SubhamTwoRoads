#!/usr/bin/env python

"""
Utility to get information about shortcode, min_price_increment, tick_change_date etc
"""

import datetime
import subprocess
import sys
import os

sys.path.append(os.path.expanduser('~/basetrade/'))
from walkforward.definitions import execs

def get_min_price_increment(t_shortcode,date):
    min_price_increment_cmd = [execs.execs().get_min_price_increment, t_shortcode, str(date)]
    process = subprocess.Popen(' '.join(min_price_increment_cmd), shell=True,
                               stderr=subprocess.PIPE,
                               stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')
    errcode = process.returncode

    return out

def get_tick_ratio_for_dates(t_shortcode,date1,date2):
    """
    
    :param t_shortcode: 
    :param date1: 
    :param date2: 
    :return: 
    """ 
    "Calculating the tick_change so that we can scale up or down the model stdev based on tick change; (model_stdev*tick_change)"
    min_price_increment_date1 = float(get_min_price_increment(t_shortcode, date1))

    min_price_increment_date2 = float(get_min_price_increment(t_shortcode,date2))

    tick_ratio = min_price_increment_date1/min_price_increment_date2
    
    return tick_ratio

