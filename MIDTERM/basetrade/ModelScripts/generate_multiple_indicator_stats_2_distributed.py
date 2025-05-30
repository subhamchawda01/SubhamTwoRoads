#!/usr/bin/env python

# \file ModelScripts/generate_indicator_stats_2+pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551

''' This script generates the correlation for a product/shc/shc and tp as per the user input between the start date and end date
and stores it in DB IndStats
'''

import sys
import os
import shutil
import argparse

sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.definitions.execs import execs
from walkforward.utils import date_utils
from pylib.indstats_db_access_manager import *
from walkforward.utils.run_exec import exec_function
from walkforward.definitions.execs import paths
from PyScripts.generate_dates import get_traindates

def get_unique_gsm_id():
    """
    # gets the uniques gsm id date +%N
    :return:
    """
    return exec_function("date +%N")[0].strip()


def ExecuteCommands(independent_parallel_commands_, distributed_):
    """
    # Executes the commands in distributed or non distributed fashion

    :param independent_parallel_commands_:
    :param distributed_:
    :return:
    """
    if int(distributed_) == 1:
        # get the name for command file
        unique_gsm_id_ = get_unique_gsm_id()
        command_file = SHARED_CMDS_LOC + unique_gsm_id_
        CMDFILE = open(command_file, "w")
        for cmd in independent_parallel_commands_:
            CMDFILE.write(cmd + "\n")
        CMDFILE.close()
        print(command_file)
        distributed_cmd = execs().celery_run_job_exec + " -n dvctrader -m 1 -f " + \
            command_file + " -s 1 -d indicator_stats_" + shc
        output_lines = exec_function(distributed_cmd)[0].strip()
        print(output_lines)
    else:
        for cmd in independent_parallel_commands_:
            print(cmd)
            os.system(cmd)


# generate the parser and add the arguments
parser = argparse.ArgumentParser()

parser.add_argument("--start_date", "-sd", help="date to check", required=True)
parser.add_argument("--shortcode", "-s", help="date to check", required=True)
parser.add_argument("--end_date", "-ed", help="date to check", required=True)
parser.add_argument("--recompute", "-r", help="date to check", default="0")
parser.add_argument("--distributed", "-d", help="date to check", default=0)
parser.add_argument("--tp", "-t", help="TimePeriod")
parser.add_argument("--pred_dur", "-pd", help="Pred Duration")
parser.add_argument("--pred_algo", "-pa", help="Pred Algo")
parser.add_argument("--filter_name", "-f", help="Filter")
parser.add_argument("--basepx", "-bp", help="Basepx")
parser.add_argument("--futpx", "-fp", help="Futpx")


args = parser.parse_args()

shc = args.shortcode
start_date = args.start_date
end_date = args.end_date
recompute = args.recompute
distributed_ = int(args.distributed)

tp = args.tp
pred_dur = args.pred_dur
pred_algo = args.pred_algo
filter_name = args.filter_name
basepx = args.basepx
futpx = args.futpx

unique_gsm_id_ = get_unique_gsm_id()
work_dir_ = paths().shared_ephemeral_indicator_logs + unique_gsm_id_

if not((os.path.exists(work_dir_))):
    os.makedirs(work_dir_)
else:
    shutil.rmtree(work_dir_)
    os.makedirs(work_dir_)

print("Working Directory : " + work_dir_)

# Initiate the distributed executor and the dir to make the command file
SHARED_CMDS_LOC = "/media/shared/ephemeral17/commands/"
if not((os.path.exists(SHARED_CMDS_LOC))):
    os.makedirs(SHARED_CMDS_LOC)


# Create and instance of IndStatsDBAcessManager to query the database
IndStatsObject = IndStatsDBAcessManager()
IndStatsObject.open_conn()

# It expects the user to provide either all of pred_dur, pred_algo, filter_name, basepx, futpx or just shc or
# (shc and tp), hence the checks
# It then queries the database and gets the map of product to prod_indc_id^indicator accordingly
if pred_dur is None and pred_algo is None and filter_name is None and basepx is None and futpx is None:
    if tp is None:
        # print "Continuing with shortcode, ignoring preddur, predalgo, filter, tp, basepx and futpx"
        product_prod_indc_n_indc_id = IndStatsObject.get_map_prod_prod_indc_n_indc_id_for_shc(shc)
    else:
        product_prod_indc_n_indc_id = IndStatsObject.get_map_prod_prod_indc_n_indc_id_for_shc_n_tp(shc, tp)
elif pred_dur is None or pred_algo is None or filter_name is None or basepx is None or futpx is None:
    print("Either Provide all of pred_dur, pred_algo, filter_name, basepx, futpx or just shc or (shc and tp)")
    sys.exit()
else:
    product_prod_indc_n_indc_id = IndStatsObject.get_map_prod_prod_indc_n_indc_id(
        shc, tp, pred_dur, pred_algo, filter_name, basepx, futpx)

# close the connection to DB
IndStatsObject.close_conn()


for product in list(product_prod_indc_n_indc_id.keys()):
    # Initiate the independent parallel commands to store here
    independent_parallel_commands_ = []

    # split the product to get the individual components
    reg_info = product.split("^")
    start_end_hr_ = reg_info[1]
    start_end_hr = start_end_hr_.strip().split("-")
    datagen_start_hhmm_ = start_end_hr[0]
    datagen_end_hhmm_ = start_end_hr[1]
    pred_dur_ = reg_info[2]
    predalgo_ = reg_info[3]
    filter_ = reg_info[4]
    basepx = reg_info[5]
    futpx = reg_info[6]

    dates = get_traindates(min_date=start_date, max_date=end_date)
    datesfile_path = os.path.join(work_dir_, "dates_" + product)
    with open(datesfile_path, "w") as fp:
        fp.write("\n".join(list(map(str, dates))))
        fp.close()
    # create and print the main ilist file so the user can check
    main_ilist = work_dir_ + "/ilist_" + shc + "_" + datagen_start_hhmm_ + "_" + datagen_end_hhmm_ + "_" + pred_dur_ + \
        "_" + predalgo_ + "_" + filter_ + "_" + basepx + "_" + futpx
    print(("Main ilist : " + main_ilist))

    ilist_to_correlation_to_DB_exec = execs().ilist_to_correlation_to_DB
    # # collect the commands for each date
    # for date in dates:
    #     cmd = " ".join([ilist_to_correlation_to_DB_exec, shc, str(date), datagen_start_hhmm_, datagen_end_hhmm_, pred_dur_, predalgo_,
    #                    filter_, basepx, futpx, recompute, work_dir_])
    #     independent_parallel_commands_.append(cmd)
    #
    # # Execute the commands
    # ExecuteCommands(independent_parallel_commands_, distributed_)

    cmd = " ".join([ilist_to_correlation_to_DB_exec, shc, datesfile_path, datagen_start_hhmm_, datagen_end_hhmm_, pred_dur_,
                    predalgo_, filter_, basepx, futpx, recompute, work_dir_])
    print(cmd)
    os.system(cmd)
