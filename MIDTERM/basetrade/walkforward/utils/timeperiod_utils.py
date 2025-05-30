#!/usr/bin/env python

"""
Utility to get functions related to timeperiod
"""

import time
from walkforward.definitions import execs
from walkforward.utils.run_exec import exec_function


def is_pool_in_tp(pool, tperiod):
    """
    # if pool(start,end-time) is contained in the tp ,returns true, else false
    :return: true/false
    """

    pool_tokens = pool.split('-')
    assert len(pool_tokens) >= 2

    [tp_start, tp_end] = ['CET_800', 'EST_1600']
    tp_tag = ''

    if tperiod == 'AS_MORN':
        [tp_start, tp_end] = ['HKT_900', 'HKT_1200']
    elif tperiod == 'AS_DAY':
        [tp_start, tp_end] = ['HKT_1200', 'HKT_1700']
    elif tperiod == 'EU_MORN_DAY':
        [tp_start, tp_end] = ['CET_700', 'EST_930']
    elif tperiod == 'EUS_MORN_DAY':
        [tp_start, tp_end] = ['CET_800', 'EST_1600']
    elif tperiod == 'US_EARLY_MORN':
        [tp_start, tp_end] = ['EST_600', 'EST_830']
    elif tperiod == 'US_MORN':
        [tp_start, tp_end] = ['EST_700', 'EST_945']
    elif tperiod == 'US_DAY':
        [tp_start, tp_end] = ['EST_915', 'EST_1700']
    elif tperiod == 'US_MORN_DAY':
        [tp_start, tp_end] = ['EST_600', 'EST_1700']
    elif '-' in tperiod:
        tp_tokens = tperiod.split('-')
        [tp_start, tp_end] = tp_tokens[0:2]
        if len(tp_tokens) > 2:
            tp_tag = tp_tokens[2]

    tp_start = int(exec_function(execs.execs().get_utc_hhmm + ' ' + tp_start)[0].strip())
    tp_end = int(exec_function(execs.execs().get_utc_hhmm + ' ' + tp_end)[0].strip())

    [p_start, p_end] = pool_tokens[0:2]
    p_start = int(exec_function(execs.execs().get_utc_hhmm + ' ' + p_start)[0].strip())
    p_end = int(exec_function(execs.execs().get_utc_hhmm + ' ' + p_end)[0].strip())
    p_tag = ''
    if len(pool_tokens) > 2:
        p_tag = pool_tokens[2]

    if (p_start >= tp_start and p_end <= tp_end) and (tp_tag == '' or tp_tag == p_tag):
        return True
    else:
        return False
