#!/usr/bin/env python

import os
import sys
import json
import getpass
import subprocess
import argparse
import time

sys.path.append(os.path.expanduser('~/basetrade/'))
from grid.client.api import GridClient
from walkforward.definitions import execs
from walkforward.utils.process_pduration import get_duration_in_algospace
from walkforward.utils.prepare_rdata import convert_t2d
from walkforward.utils.process_rdata import apply_filters
from PyScripts.generate_dates import get_traindates
from walkforward.utils.date_utils import calc_prev_week_day


GRID_URL = "http://10.1.4.15:5000"
datagen_script = "/home/" + getpass.getuser() + "/basetrade_install/bin/datagen"

usage_ = "USAGE : <script> <shortcode> <ilist> <start_date> <num_days_lookback> <start_time> <end_time> " \
         "<dgen_msecs> <dgen_l1events> <dgen_trades> <to_print_on_eco> <pred_duration> <pred_algo> <filter> " \
         "<work_dir>"
parser = argparse.ArgumentParser(description="Generates regdata")
parser.add_argument('shortcode', metavar='shortcode', type=str, help='shortcode to generate data')
parser.add_argument('ilist', metavar='ilist', type=str, help='ilist filename')
parser.add_argument('end_date', metavar='end_date', type=str, help='end date of data window')
parser.add_argument('num_days', metavar='num_days', type=int, help='number of days to generate data')
parser.add_argument('start_time', metavar='start_time', type=str, help='start time for data')
parser.add_argument('end_time', metavar='end_time', type=str, help='end time for data')
parser.add_argument('msecs', metavar='msecs', type=str, help='msecs for sampling')
parser.add_argument('l1events', metavar='l1events', type=str, help='events for sampling')
parser.add_argument('trades', metavar='trades', type=str, help='trades for sampling')
parser.add_argument('eco', metavar='eco', type=str, help='eco mode')
parser.add_argument('pred_duration', metavar='pred_duration', type=str, help='pred duration for delta y')
parser.add_argument('pred_algo', metavar='pred_algo', type=str, help='pred algo for delta y')
parser.add_argument('regdata_filter', metavar='regdata_filter', type=str, help='filtering for regdata')
parser.add_argument('work_dir', metavar='work_dir', type=str, help='work directory to store regdata')
parser.add_argument('--sum_vars', dest='compute_sum_vars', help='to compute sum vars', default=0, const=1, required=False,
                    action='store_const')
parser.add_argument('--no-grid', dest='no_grid',
                    help='whether to use grid or not', default=False,
                    required=False, action='store_true')
args = parser.parse_args()

if len(sys.argv) < 15:
    print("USAGE:", usage_)
    sys.exit(1)

if args.no_grid is False and ('GRID_USERNAME' not in os.environ or os.environ['GRID_USERNAME'] == "" or 'GRID_PASSWORD' not in os.environ or os.environ['GRID_PASSWORD'] == ""):
    grid_user = input("Enter username: ")
    if grid_user == "":
        print ("Enter a valid username")
        sys.exit(1)
    password = getpass.getpass("Enter password: ")
    os.environ['GRID_USERNAME']=grid_user
    os.environ['GRID_PASSWORD']=password


shortcode = args.shortcode
ilist = args.ilist
end_date = args.end_date
num_days = args.num_days
start_time = args.start_time
end_time = args.end_time
msecs = args.msecs
l1events = args.l1events
trades = args.trades
eco = args.eco
pred_duration = args.pred_duration
pred_algo = args.pred_algo
regdata_filter = args.regdata_filter
work_dir = args.work_dir
compute_sum_vars = args.compute_sum_vars

temp_ilist = "/media/shared/ephemeral16/" + getpass.getuser() + "/temp_ilist_" + str(int(time.time() * 1000))
os.system("cp " + ilist + " " + temp_ilist)
os.system("rm -rf " + work_dir)
os.system("mkdir -p " + work_dir)
catted_regdata_filename = os.path.join(work_dir, "catted_regdata_outfile")
filtered_regdata_filename = os.path.join(work_dir, "filtered_regdata_filename")

dates = list(map(int, get_traindates(max_date=end_date, n=num_days)))

duration_in_algo_space = get_duration_in_algospace(shortcode, pred_duration, end_date, pred_algo, start_time, end_time)


datagen_json = {"ilist": temp_ilist, "job": "generate_data", "msecs": "10000", "l1events": "0", "trades": "0",
                "start_time": str(start_time), "end_time": str(end_time),
                "dates": list(map(str, dates)), "eco_mode": "0"}
if compute_sum_vars == 1:
    datagen_json['stats_args'] = "SUM_VARS"
else:
    datagen_json['stats_args'] = None

if args.no_grid:
    for date in dates:
        tdata_file = os.path.join(work_dir, "tdata_" + str(date))
        if compute_sum_vars == 1:
            tdata_file = "SUM_VARS@" + tdata_file
        datagen_cmd = [datagen_script, ilist, str(date), start_time, end_time, "2222", tdata_file, msecs, l1events,
                       trades, eco]
        os.system(" ".join(datagen_cmd))
else:
    grid_client = GridClient(server_url=GRID_URL, username=os.getenv("GRID_USERNAME"),
                             password=os.getenv("GRID_PASSWORD"), grid_artifacts_dir=work_dir)
    output_directory = grid_client.submit_job(json.dumps(datagen_json))
    datagen_artifacts_directory = os.path.join(output_directory, "artifacts", "datagen")
    for date in dates:
        date_file = os.path.join(datagen_artifacts_directory, str(date) + ".txt")
        tdata_file = os.path.join(work_dir, "tdata_" + str(date))
        if os.path.exists(date_file):
            os.system("mv " + date_file + " " + tdata_file)
    os.system("rm -rf " + datagen_artifacts_directory)

for date in dates:
    tdata_file = os.path.join(work_dir, "tdata_" + str(date))
    rdata_file = os.path.join(work_dir, "rdata_" + str(date))
    if os.path.exists(tdata_file):
        out, err, errcode = convert_t2d(shortcode, date, ilist, tdata_file, str(duration_in_algo_space),
                                        pred_algo,
                                        rdata_file, regdata_filter)

        if len(err) > 0:
            print("Error: reported in t2d module", err)
        if errcode != 0:
            print("Error: received non zero exitcode fro, t2d module", errcode)
        os.system("cat " + rdata_file + " >> " + catted_regdata_filename)

out, err, errcode = apply_filters(shortcode, catted_regdata_filename, regdata_filter, filtered_regdata_filename)

if len(err) > 0:
    print("Error: reported in filters module", err)
if errcode != 0:
    print("Error: received non zero exitcode from, filters module", errcode)
