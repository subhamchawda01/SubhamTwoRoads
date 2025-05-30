#!/usr/bin/env python
"""
Script to generate statistics for a portfolio
"""

import os
import sys
import subprocess
import argparse
import datetime


sys.path.append(os.path.expanduser('~/basetrade/'))
from get_config_n_uts_for_queryid_n_date import get_date_to_config_to_uts_map_for_queries_n_dates
from walkforward.definitions import execs
from walkforward.wf_db_utils.db_handles import connection
from walkforward.utils.sql_str_converter import sql_out_to_str
from walkforward.utils.get_list_of_dates import get_list_of_dates
from pylib.pnl_modelling_utils.load_config_date_pnl_map import get_config_uts_n_date_pnl
from scripts.get_portfolio_stats_sessionwise import get_dated_utsscaled_sim_pnls


def get_real_datewise_pnl_(shc_, session_, date_, num_days_):
    """

    :param product_to_config_dict_: 
    :param date_: 
    :param history_: 
    :return: 
    """

    script_ = execs.execs().calc_prev_week_day
    cmd1_ = script_ + " " + date_ + " " + num_days_
    proc = subprocess.Popen(cmd1_, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    out, err = proc.communicate()
    out = out.decode('utf-8')
    startdate_ = out.strip()
    enddate_ = date_

    date_to_real_pnl_map = {}
    # fetch_query = "select date,pnl,volume from RealPNLS where shortcode = '" + shc_ + "' and session = '" + session_ + "' and volume != 'NULL' and date >= " + str(startdate_) +  " and date <= " + str(enddate_) + ";"
    fetch_query = "select date,pnl from RealPNLS where shortcode = '" + shc_ + "' and session = '" + \
                  session_ + "' and date >= " + str(startdate_) + " and date <= " + str(enddate_) + \
                  " and pnl is not NULL;"
    cursor = connection().cursor()
    cursor.execute(fetch_query)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    for rows in data:
        dt = rows[0]
        pnl = rows[1]
        if (pnl != None):
            date_to_real_pnl_map[dt] = pnl

    return date_to_real_pnl_map


parser = argparse.ArgumentParser()
parser.add_argument('-shc', dest='shortcode', help="Shortcode", type=str, required=True)
parser.add_argument('-ed', dest='end_date', type=str, required=True)
parser.add_argument('-num_days', dest='num_days', type=str, required=True)
parser.add_argument('-session', dest='session', type=str, required=True)
parser.add_argument('-qsid', dest='query_start_id', type=str, required=True)
parser.add_argument('-qeid', dest='query_end_id', type=str, required=True)

args = parser.parse_args()
date_to_real_pnls = get_real_datewise_pnl_(args.shortcode, args.session, args.end_date, args.num_days)
date_pnl_map = get_dated_utsscaled_sim_pnls(
    args.shortcode, args.query_start_id, args.query_end_id, args.end_date, args.num_days)

print("Date      SimPnL RealPnl")
dates = list(map(str, date_to_real_pnls.keys()))
dates = sorted(dates, key=lambda x: datetime.datetime.strptime(x, '%Y%m%d'))
dates = list(map(int, dates))
for dt in dates:
    if dt in date_pnl_map:
        print("%d %7d %7d" % (dt, date_to_real_pnls[dt], int(date_pnl_map[dt])))
    else:
        print("%d %7d %7d" % (dt, date_to_real_pnls[dt], 000))
