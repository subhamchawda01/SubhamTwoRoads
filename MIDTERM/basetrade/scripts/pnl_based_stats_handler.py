#!/usr/bin/env python
import sys
import os
import argparse

sys.path.append(os.path.expanduser('~/basetrade/'))

from scripts.ind_pnl_based_stats import *

parser = argparse.ArgumentParser()

parser = argparse.ArgumentParser()
parser.add_argument("--shc", "-s", help="Shortcode", required=True)
parser.add_argument("--start_time", "-st", help="Start time", required=True)
parser.add_argument("--end_time", "-et", help="End time", required=True)
parser.add_argument("--ilist", "-i", help="Ilist")
parser.add_argument("--start_date", "-sd", type=int, help="Start Date")
parser.add_argument("--end_date", "-ed", type=int, help="End Date")
parser.add_argument("--mode", "-m", help="ADD_PROD/ADD_INDICATORS/GENERATE/FETCH", required=True)

args = parser.parse_args()
shc = args.shc
start_time = args.start_time
end_time = args.end_time

mode = args.mode
if mode == "ADD_PRODUCT":
    insert_product(shc, start_time, end_time)
elif mode == "ADD_INDICATORS":
    if args.ilist is None:
        print("ILIST required")
        sys.exit(1)
    insert_product(shc, start_time, end_time)
    ilist = args.ilist
    insert_indicators_for_product(shc, start_time, end_time, ilist)
elif mode == "GENERATE":
    if args.start_date is None or args.end_date is None:
        print("START DATE and END DATE required")
        sys.exit(1)
    generate_stats_for_product(shc, start_time, end_time, start_date=args.start_date, end_date=args.end_date)
elif mode == "FETCH":
    if args.ilist is None:
        print("ILIST required")
        sys.exit(1)
    if args.start_date is None or args.end_date is None:
        print("START DATE and END DATE required")
        sys.exit(1)
    ilist = args.ilist

    pnl_stats = fetch_pnl_stats_for_ilist_dates(shc, start_time, end_time, ilist,
                                                 start_date=args.start_date, end_date=args.end_date)

    print(pnl_stats)
