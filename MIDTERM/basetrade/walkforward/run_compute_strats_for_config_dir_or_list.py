#!/usr/bin/env python


"""
Given a directory or list and date range, it computes the strats for all configs in it
"""

import os
import sys
import argparse
import getpass

sys.path.append(os.path.expanduser('~/basetrade/'))
sys.path.append(os.path.expanduser('~/grid/'))

from walkforward.compute_strat_for_config_and_date import compute_strat_for_config_and_date
from walkforward.utils.get_all_files_in_dir import get_all_files_in_dir


parser = argparse.ArgumentParser()
parser.add_argument('-c', dest='dir_or_list',
                    help="Directory/File containing walkforward configs", type=str, required=True)
parser.add_argument('-d', dest='tradingdate', help="date/end-date to generate the strat for", type=int, required=True)
parser.add_argument('-l', dest='lookback', help='look_back_days to generate the strats for',
                    default=1, type=int, required=True)
parser.add_argument('-r', dest='rewrite', help='rewrite if strt already exists', default=0, type=int)


args = parser.parse_args()
rewrite = args.rewrite != 0

if 'GRID_USERNAME' not in os.environ or os.environ['GRID_USERNAME'] == "" or 'GRID_PASSWORD' not in os.environ or os.environ['GRID_PASSWORD'] == "":
    grid_user = input("Enter grid username")
    if grid_user == "":
        print("Enter valid username")
        sys.exit(1)
    password = getpass.getpass("Enter grid password")
    os.environ['GRID_USERNAME'] = grid_user
    os.environ['GRID_PASSWORD'] = password

filelist = []

if os.path.isdir(args.dir_or_list):
    get_all_files_in_dir(args.dir_or_list, filelist)

elif os.path.exists(args.dir_or_list):
    contents = []
    with open(args.dir_or_list) as file:
        contents = file.read().splitlines()

    for line in contents:
        filelist.append(line)

else:
    print("Not a valid directory or file " + args.dir_or_list)

if len(filelist) > 0:
    for config in filelist:
        print("Computing for config: " + config + " date: " + str(args.tradingdate))
        try:
            compute_strat_for_config_and_date(config, args.tradingdate, args.lookback,
                                              rewrite)  # TODO here we would want to use celery calls
        except Exception as e:
            print(str(e))
            print("\n")
