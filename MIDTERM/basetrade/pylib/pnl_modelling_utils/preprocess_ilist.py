#!/usr/bin/env python
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
"""
Script to preprocess the main ilist for pnl_modelling and get a shorter ilist
"""

import subprocess
import warnings
import os
import sys
import numpy as np
import pandas as pd
import getpass
import time
import json

sys.path.append(os.path.expanduser('~/basetrade/'))


from grid.client.api import GridClient
from walkforward.utils.process_rdata import apply_filters
from walkforward.definitions import execs
from walkforward.utils.process_pduration import get_duration_in_algospace
from walkforward.utils.prepare_rdata import convert_t2d
from walkforward.utils.learn_signal_weights import run_method
from walkforward.wf_db_utils.utils import FileExistsWithSize
from scripts.stats_based_ind_sel import stats_based_ind_sel
GRID_URL = "http://10.1.4.15:5000"


"""
This function creates a temporary directory, iterates over the lookback days, and generates data from ilist, over the lookback days, 
applies filter on the data, generates the model
"""


def build_model(shortcode, ilist, dates_for_preprocessing, start_time, end_time,
                datagen_args, regdata_args, regdata_process_filter,
                reg_string, sign_check_string, parent_work_dir=None):

    work_dir = parent_work_dir
    if work_dir is None:
        work_dir = "/spare/local/" + getpass.getuser() + "/PNL_MODELLING_PREPROCESSING/" + shortcode + \
            "/" + str(int(time.time() * 1000)) + "/"
        os.system("rm -rf " + work_dir)
        os.system("mkdir --parents " + work_dir)
    print("Work dir for preprocessing is " + work_dir)

    catted_file = work_dir + "catted_rdata_file"
    if FileExistsWithSize(catted_file):
        open(catted_file, 'w').close()

    filtered_file = work_dir + "filtered_rdata_file"
    if FileExistsWithSize(filtered_file):
        open(filtered_file, 'w').close()

    reg_output_file = work_dir + "reg_output_file"
    if FileExistsWithSize(reg_output_file):
        open(reg_output_file, 'w').close()

    final_model_file = work_dir + "final_model_file"
    if FileExistsWithSize(final_model_file):
        open(final_model_file, 'w').close()

    dates_file = work_dir + "dates_file"
    if FileExistsWithSize(dates_file):
        open(dates_file, 'w').close()

    param = work_dir + "param_file"
    if FileExistsWithSize(param):
        open(param, 'w').close()

    dfh = open(dates_file, 'w')
    for date in dates_for_preprocessing:
        dfh.write("%s\n" % date)
    dfh.close()

    regress_exec = (reg_string.split())[0]
    if regress_exec == "FAST_FSRR":
        stats_based_ind_sel(shortcode, ilist, dates_for_preprocessing, start_time, end_time, regdata_args, regdata_process_filter, reg_string, final_model_file)
        if FileExistsWithSize(final_model_file):
            return work_dir, final_model_file
        else:
            min_corr, max_corr, _, max_model_size = reg_string.split()[1:]
            reg_string = "FSRR 0.1 {} 0 0 {} {}".format(min_corr, max_corr, max_model_size)

    datagen_args = datagen_args.split()
    ptime = datagen_args[0]
    events = datagen_args[1]
    trades = datagen_args[2]

    regdata_args = regdata_args.split()
    if regdata_args[1] == "VOL" or regdata_args[1] == "STDEV":
        feature = regdata_args[1]
        pred_algo = regdata_args[0]
        duration_in_feature_space = regdata_args[2]
        pred_dur_cmd = [execs.execs().dynamic_pred_dur, shortcode, str(date), str(len(dates_for_preprocessing)),
                        start_time, end_time, feature, duration_in_feature_space]
        p_feat = subprocess.Popen(' '.join(pred_dur_cmd), shell=True,
                                  stderr=subprocess.PIPE,
                                  stdout=subprocess.PIPE)
        out, err = p_feat.communicate()
        if out is not None:
            out = out.decode('utf-8')
        if err is not None:
            err = err.decode('utf-8')
        errcode = p_feat.returncode
        if len(err) > 0:
            raise ValueError("Error in retrieving dynamic pred duration")

        duration_in_tspace = float(out)
        print(out)

    else:
        pred_algo = regdata_args[0]
        duration_in_tspace = regdata_args[1]
    print("Dynamic Predduration ", duration_in_tspace)

    # DB thinks we should get per date before calling RegData converstion ?
    duration_in_algo_space = get_duration_in_algospace(shortcode, duration_in_tspace, str(date), pred_algo, start_time,
                                                       end_time)
    print(duration_in_algo_space)

    """# start of data loop; this is the implementation that data is generated only once; not reuquired as of now
    dates_to_run_grid = []
    for t_date in dates_for_preprocessing:
        rdata_file = work_dir + "rdata_file_" + t_date
        if not FileExistsWithSize(rdata_file):
            dates_to_run_grid.append(t_date)"""

    # end of data loop
    temp_ilist = "/media/shared/ephemeral16/" + getpass.getuser() + "/temp_ilist_" + str(int(time.time() * 1000))
    os.system("cp " + ilist + " " + temp_ilist)
    datagen_grid_json = {"job": "generate_data", "ilist": temp_ilist, "start_time": str(start_time), "end_time": str(end_time), "msecs": str(ptime), "l1events": str(events),
                         "trades": str(trades), "eco_mode": "0", "stats_args": None,
                         "dates": list(map(str, dates_for_preprocessing))}

    grid_client = GridClient(server_url=GRID_URL, username=os.getenv("GRID_USERNAME"),
                             password=os.getenv("GRID_PASSWORD"), grid_artifacts_dir=work_dir)
    output_directory = grid_client.submit_job(json.dumps(datagen_grid_json), True)
    os.system("rm " + temp_ilist)
    datagen_artifacts_directory = os.path.join(output_directory, "artifacts", "datagen")
    for date in dates_for_preprocessing:
        date_file = os.path.join(datagen_artifacts_directory, str(date) + ".txt")
        tdata_file = work_dir + "tdata_file_" + str(date)
        if os.path.exists(date_file):
            os.system("mv " + date_file + " " + tdata_file)

    os.system("rm -rf " + output_directory)

    for t_date in dates_for_preprocessing:
        t_date = str(t_date)
        tdata_file = work_dir + "tdata_file_" + t_date
        rdata_file = work_dir + "rdata_file_" + t_date
        if not FileExistsWithSize(rdata_file):
            out, err, errcode = convert_t2d(shortcode, t_date, ilist, tdata_file, str(duration_in_algo_space),
                                            pred_algo,
                                            rdata_file, regdata_process_filter)

            if len(err) > 0:
                print("Error: reported in t2d module", err)
            if errcode != 0:
                print("Error: received non zero exitcode fro, t2d module", errcode)

        os.system("cat " + rdata_file + " >> " + catted_file)

    out, err, errcode = apply_filters(shortcode, catted_file, regdata_process_filter, filtered_file)

    if len(err) > 0:
        print("Error: reported in filters module", err)
    if errcode != 0:
        print("Error: received non zero exitcode from, filters module", errcode)

    # In this case we don't want to learn weights by regression
    if (reg_string.split())[0] == "CORR":
        print("Method not implemented")

    out = run_method(ilist, filtered_file, reg_string, reg_output_file, final_model_file)

    # If the regress exec doesn't create a model file, dont throw error, let the modelling process continue
    if not os.path.exists(final_model_file):
        print("Failed to create model file \n")
        sys.exit()

    return work_dir, final_model_file


"""
    sign_check_passed = True
    if sign_check_string == "1":
        sign_check_passed = check_sign_with_ilist(ilist, final_model_file)
"""
