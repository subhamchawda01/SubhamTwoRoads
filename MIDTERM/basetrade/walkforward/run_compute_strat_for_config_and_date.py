#!/usr/bin/env python


"""
Runs the compute model-param for a given config
"""
from __future__ import print_function
import os
import sys
import argparse
import getpass
import time

sys.path.append(os.path.expanduser('~/basetrade/'))
sys.path.append(os.path.expanduser('~/grid/'))

from walkforward.compute_strat_for_config_and_date import compute_strat_for_config_and_date
from walkforward.wf_db_utils.db_handles import set_backtest
from walkforward.wf_db_utils.fetch_config_details import fetch_config_details
from walkforward.utils.file_utils import clean_parent_dir

# set_backtest(True)


parser = argparse.ArgumentParser()
parser.add_argument('-c', dest='configname', help="the walk-forward-config identifier", type=str, required=True)
parser.add_argument('-d', dest='tradingdate', help="date/end-date to generate the strat for", type=int, required=True)
parser.add_argument('--use-backtest', dest='use_backtest',
                    help='whether to use backtest DB or not', default=False)
parser.add_argument('-l', dest='lookback', help='look_back_days to generate the strat for',
                    default=1, type=int, required=True)
parser.add_argument('-r', dest='rewrite', help='rewrite if strt already exists', default=0, type=int)
args = parser.parse_args()

if 'GRID_USERNAME' not in os.environ or os.environ['GRID_USERNAME'] == "" \
        or 'GRID_PASSWORD' not in os.environ or os.environ['GRID_PASSWORD'] == "":
    grid_user = input("Enter grid username: ")
    if grid_user == "":
        print("Enter valid username")
        sys.exit(1)
    password = getpass.getpass("Enter grid password: ")
    os.environ['GRID_USERNAME'] = grid_user
    os.environ['GRID_PASSWORD'] = password

rewrite = args.rewrite != 0
if args.use_backtest or 'USE_BACKTEST' in os.environ:
    set_backtest(True)
    os.environ['USE_BACKTEST'] = "1"

cfg = fetch_config_details(args.configname)
work_dir = "/spare/local/" + getpass.getuser() + "/WF/" + cfg.shortcode + "/Parent/" + str(int(time.time() * 1000)) + "/"
os.system("mkdir --parents " + work_dir)
stdout_file = work_dir + "stdout.txt"
stderr_file = work_dir + "stderr.txt"
print(stdout_file)
try:
    compute_strat_for_config_and_date(args.configname, args.tradingdate, args.lookback, rewrite, work_dir)
    clean_parent_dir(work_dir)
except Exception as e:
    clean_parent_dir(work_dir)
    print("Error in compute_strat_for_config " + str(e) + "\n")
    sys.exit()
