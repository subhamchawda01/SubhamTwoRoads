#!/usr/bin/env python

from __future__ import print_function
import sys
import os
import argparse
import pandas as pd
import random
import time
import shutil

sys.path.append(os.path.expanduser('~/basetrade/'))
from walkforward.utils.date_utils import calc_next_week_day
from walkforward.utils.date_utils import calc_prev_week_day
from walkforward.definitions import execs
from walkforward.wf_db_utils.fetch_strat_from_config_and_date import fetch_strat_from_config_and_dates
from walkforward.wf_db_utils.db_handles import connection
from walkforward.wf_db_utils.db_handles import set_backtest
from walkforward.utils.sql_str_converter import sql_out_to_str
from walkforward.wf_db_utils.fetch_structured_strat_from_config_struct_and_date import fetch_structured_strat_from_name_and_dates


# set_backtest(true)


def print_strat_from_multiple_config_multiple_date(file_with_configs, start_date, end_date, use_days_file=None, skip_days_file=None, work_dir=None, days_to_look=None):
    """

    get strats for the a file with config list and between start and end date

    file_with_configs: str
            path to the file having config names
    start_date: int
            start date
    end_date: int
            end date
    use days file: str
            file path to the file having lsit of dates to use
    skip_days_file: str
            file path to the file having list of dates to skip
    work_dir : str
            Location where output files needs to be generated

    return:
            prints location of the path where the output files are created.
            for each date, a folder is created which has 2 files, one for the list of configs and one for their corresponding strategy lines
    """

    with open(file_with_configs) as f:
        file_with_configs = f.read().splitlines()

    date = start_date
    trade_date_list = []

    if use_days_file is not None and use_days_file != "":
        with open(use_days_file) as f:
            trade_date_list = list(map(int, f.read().splitlines()))
    else:
        while date <= end_date:
            trade_date_list.append(date)
            date = calc_next_week_day(date, 1)

    skip_dates = []
    if skip_days_file is not None and skip_days_file != "":
        with open(skip_days_file) as f:
            skip_dates = list(map(int, f.read().splitlines()))

    final_trade_date_list = [dt for dt in trade_date_list if dt not in skip_dates]
    #print(final_trade_date_list)

    if (len(final_trade_date_list) == 0) or (len(file_with_configs) == 0):
        print("Config List/Date List not provided or incorrect. Please check the input.")
        sys.exit(123)

    if days_to_look is None:
        days_to_look = 15
    all_output_list = []
    cursor = connection().cursor()
    for conf in file_with_configs:
        search_query = ("SELECT is_structured FROM wf_configs where cname = \"%s\" " % (conf))
        cursor.execute(search_query)
        data = cursor.fetchall()
        data = sql_out_to_str(data)
        is_structured = 0
        if len(data) > 0:
            is_structured = int(data[0][0])
        #print("IS_STRUCTURED :")
        #print(is_structured)
        if is_structured < 1:
            all_output_list.append(fetch_strat_from_config_and_dates(conf, final_trade_date_list, days_to_look))
        else:
            all_output_list.append(fetch_structured_strat_from_name_and_dates(
                conf, final_trade_date_list, days_to_look, is_structured))

    output_df = pd.concat(all_output_list, ignore_index=True)

    if work_dir is None:
        temp_location = execs.get_temp_location()
        work_dir = temp_location + "stratline_output_" + str(random.randrange(1, 100000)) + "/"
        while os.path.isdir(work_dir):
            if (time.time() - os.path.getmtime(work_dir)) < 86400:
                work_dir = temp_location + "stratline_output_" + str(random.randrange(1, 100000)) + "/"
            else:
                shutil.rmtree(work_dir)

    os.system("mkdir --parents " + work_dir)

    if output_df.shape[0] == 0:
        print("No strat is fetched for the given set of configs and dates. Please check input once.")
        sys.exit(123)
    else:
        for date in output_df['Date'].drop_duplicates().tolist():
            date_dir = work_dir + "/" + str(int(date)) + "/"
            os.system("mkdir --parents " + date_dir)
            filtered_df = output_df[output_df['Date'] == date]
            filtered_list = filtered_df[['ConfigName', 'StrategyLine']].values.tolist()
            for config_strat_pair in filtered_list:
                config_name = config_strat_pair[0]
                strat_line = config_strat_pair[1]
                fopen = open(date_dir + config_strat_pair[0], 'w')
                fopen.write(strat_line)
                fopen.close()

    return work_dir


if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument('-cfile', dest='file_with_configs', help="File with list of configs", type=str, required=True)
    parser.add_argument('-sd', dest='start_date', help="Start Date", type=int, required=True)
    parser.add_argument('-ed', dest='end_date', help='End Date', type=int, required=True)
    parser.add_argument('-use', dest='use_days_file', help='Use Days file', type=str, required=False)
    parser.add_argument('-skip', dest='skip_days_file', help='Skip Days file', type=str, required=False)
    parser.add_argument('-work_dir', dest='work_dir', help='Working Directory Location', type=str, required=False)
    parser.add_argument('-look_days', dest='days_to_look',
                        help='Number of past models to look for', type=int, required=False)
    parser.add_argument('-b', dest='use_backtest', help='Use backtest results', type=int, required=False)
    args = parser.parse_args()

    if args.use_backtest == 1 or 'USE_BACKTEST' in os.environ:
        set_backtest(True)
        os.environ['USE_BACKTEST'] = "1"

    out_dir = print_strat_from_multiple_config_multiple_date(
        args.file_with_configs, args.start_date, args.end_date, args.use_days_file, args.skip_days_file, args.work_dir, args.days_to_look)
    print("OUTPUT_DIR: %s" % (out_dir))
