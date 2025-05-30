#!/usr/bin/env python
"""
simple tool to get trigger dates for type6 configs
it also print if the model/strat exists as expected
"""
import argparse
import sys
import os

sys.path.append(os.path.expanduser('~/basetrade/'))
from walkforward.wf_db_utils.fetch_config_details import fetch_config_id, fetch_config_details
from walkforward.wf_db_utils.fetch_strat_from_type6_config_struct_and_date import check_strat_for_config6_and_date
from walkforward.utils.date_utils import calc_prev_week_day

parser = argparse.ArgumentParser(description="lists trigger dates and info about strats/model exists")

parser.add_argument('-c', help='config name', required=True, dest='config_name')
parser.add_argument('-e', help='end date', type=int, required=True, dest='end_date')
parser.add_argument('-s', help='start_date', type=int, required=True, dest='start_date')
parser.add_argument('-p', help='print_format', type=int, default=1, required=False, dest='print_format')
args = parser.parse_args()


end_date = int(args.end_date)
start_date = int(args.start_date)
config_id = fetch_config_id(args.config_name)
date = end_date

config_struct = fetch_config_details(args.config_name)
config_struct.sanitize()

while date >= start_date:
    if config_struct.config_type == 6:
        (exists, trigger_date) = check_strat_for_config6_and_date(config_id, date)
    else:
        exit(0)

    if exists:
        if args.print_format == 1:
            print(str(trigger_date) + " exists")
    else:
        print(str(trigger_date) + " missing")

    date = calc_prev_week_day(trigger_date, 1)
