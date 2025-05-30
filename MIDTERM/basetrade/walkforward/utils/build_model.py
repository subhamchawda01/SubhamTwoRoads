#!/usr/bin/env python
# \file walkforward/build_model.py
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#

import argparse
import subprocess
import sys
import os
import shutil
import getpass
import time
import signal
import datetime
import time
import json

sys.path.append(os.path.expanduser('~/basetrade/'))
from grid.client.api import GridClient
from walkforward.utils.date_utils import calc_prev_week_day
from walkforward.utils.shortcode_utils import get_tick_ratio_for_dates
from walkforward.definitions import execs
from walkforward.wf_db_utils.utils import FileExistsWithSize
from walkforward.utils.prepare_tdata import run_datagen
from walkforward.utils.process_pduration import get_duration_in_algospace
from walkforward.utils.prepare_rdata import convert_t2d
from walkforward.utils.process_rdata import apply_filters
from walkforward.utils.learn_signal_weights import run_method, run_fill_based_model_generator
from walkforward.utils.process_model import process_model
from walkforward.utils.model_sign_check import check_sign_with_ilist
from walkforward.wf_db_utils.fetch_dump_model import fetch_model_desc_from_configid
from walkforward.wf_db_utils.fetch_dump_param import fetch_param_desc_from_configid
from walkforward.utils.get_similar_dates import get_similar_dates
from walkforward.utils.get_continuous_and_random_dates import get_continuous_and_random_dates
GRID_URL = "http://10.1.4.15:5000"


"""
This function creates a temporary directory, iterates over the lookback days, and generates data from ilist, over the lookback days, 
applies filter on the data, generates the model and validates the model to check if overfitted.
"""


def build_model(shortcode, config_id, model_for_date, num_days, start_time, end_time,
                datagen_args, regdata_args, regdata_process_filter,
                reg_string, model_process_string, sign_check_string, config_json, parent_work_dir=None):

    # we use date before model live date
    date = calc_prev_week_day(model_for_date, 1)
    config_created_date = config_json['description:'].split()[-1].replace("-", "")

    indicator_sharpe_threshold = 0.1
    model_sharpe_threshold = 0.1
    if num_days.split("-")[0] == "SIMILAR":
        dates = get_similar_dates(shortcode, num_days, date, start_time, end_time, model_for_date)
    if num_days.split('_')[0] == 'RANDOM':
        # select half of the days from lookback and half of the days randomly from last 1 year data
        lookback_continuous = int(num_days.split('_')[1]) / 2
        lookback_random = 250
        num_days_random = lookback_continuous
        dates = list(map(str, get_continuous_and_random_dates(shortcode, date, lookback_continuous,
                                                              lookback_random, num_days_random, [])))
    else:
        dates_cmd = [execs.execs().get_dates, shortcode, str(num_days), str(date)]
        process = subprocess.Popen(' '.join(dates_cmd), shell=True,
                                   stderr=subprocess.PIPE,
                                   stdout=subprocess.PIPE)
        out, err = process.communicate()
        if out is not None:
            out = out.decode('utf-8')
        if err is not None:
            err = err.decode('utf-8')
        errcode = process.returncode
        if len(err) > 0:
            raise ValueError("Error in retrieving dates")

        dates = out.split()

    work_dir = parent_work_dir
    if work_dir is None:
        work_dir = "/spare/local/" + getpass.getuser() + "/WFRM/" + shortcode + "/" + str(int(time.time() * 1000)) + "/"
        os.system("rm -rf " + work_dir)
        os.system("mkdir --parents " + work_dir)

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

    ilist = work_dir + "ilist_file"
    if FileExistsWithSize(ilist):
        open(ilist, 'w').close()

    param = work_dir + "param_file"
    if FileExistsWithSize(param):
        open(param, 'w').close()

    dfh = open(dates_file, 'w')
    for date in dates:
        dfh.write("%s\n" % date)
    dfh.close()

    model_desc = fetch_model_desc_from_configid(config_id, 0)
    param_desc = fetch_param_desc_from_configid(config_id, 0)
    f = open(ilist, 'w')
    f.write(model_desc)
    f.close()

    f = open(param, 'w')
    f.write(param_desc)
    f.close()

    regress_exec = (reg_string.split())[0]

    print(work_dir)

    # if the regress exec is to train the model using fill based modelling, call run_fill_based_model_generator,
    # that will take care of everything , datagen and training and scaling.
    if regress_exec == "FILL":
        skip_days_file = "IF"
        run_fill_based_model_generator(reg_string, shortcode, ilist, param, date, num_days, start_time, end_time,
                                       datagen_args, skip_days_file, model_process_string, final_model_file)

        if not os.path.exists(final_model_file):
            return True, work_dir, final_model_file

        processed_model_file = final_model_file

        sign_check_passed = True
        if sign_check_string == "1":
            sign_check_passed = check_sign_with_ilist(ilist, processed_model_file)

        return sign_check_passed, work_dir, processed_model_file

    datagen_args = datagen_args.split()
    ptime = datagen_args[0]
    events = datagen_args[1]
    trades = datagen_args[2]

    regdata_args = regdata_args.split()
    if regdata_args[1] == "VOL" or regdata_args[1] == "STDEV":
        feature = regdata_args[1]
        pred_algo = regdata_args[0]
        duration_in_feature_space = regdata_args[2]
        pred_dur_cmd = [execs.execs().dynamic_pred_dur, shortcode, str(date), str(num_days),
                        start_time, end_time, feature, duration_in_feature_space]
        p_feat = subprocess.Popen(' '.join(pred_dur_cmd), shell=True,
                                  stderr=subprocess.PIPE,
                                  stdout=subprocess.PIPE)
        out, err = p_feat.communicate()
        if out is not None:
            out = out.decode('utf-8')
        if err is not None:
            err = err.decode('utf-8')
        errcode = process.returncode
        if len(err) > 0:
            raise ValueError("Error in retrieving dynamic pred duration")

        duration_in_tspace = float(out)
        print(out)

    else:
        pred_algo = regdata_args[0]
        duration_in_tspace = regdata_args[1]
    print("Dynamic Predduration ", duration_in_tspace)

    # DB thinks we should get per date before calling RegData converstion ?
    duration_in_algo_space = get_duration_in_algospace(shortcode, duration_in_tspace, date, pred_algo, start_time,
                                                       end_time)
    print(duration_in_algo_space)

    # start of data loop
    dates_to_run_grid = []
    for t_date in dates:
        rdata_file = work_dir + "rdata_file_" + t_date
        if not FileExistsWithSize(rdata_file):
            dates_to_run_grid.append(t_date)

    # end of data loop
    temp_ilist = "/media/shared/ephemeral16/" + getpass.getuser() + "/temp_ilist_" + str(int(time.time() * 1000))
    os.system("cp " + ilist + " " + temp_ilist)
    datagen_grid_json = {"job": "generate_data", "ilist": temp_ilist, "start_time": str(start_time), "end_time": str(end_time), "msecs": str(ptime), "l1events": str(events),
                         "trades": str(trades), "eco_mode": "0", "stats_args": None,
                         "dates": list(map(str, dates_to_run_grid))}

    grid_client = GridClient(server_url=GRID_URL, username=os.getenv("GRID_USERNAME"),
                             password=os.getenv("GRID_PASSWORD"), grid_artifacts_dir=work_dir)
    output_directory = grid_client.submit_job(json.dumps(datagen_grid_json))
    os.system("rm " + temp_ilist)
    datagen_artifacts_directory = os.path.join(output_directory, "artifacts", "datagen")
    for date in dates_to_run_grid:
        date_file = os.path.join(datagen_artifacts_directory, str(date) + ".txt")
        tdata_file = work_dir + "tdata_file_" + str(date)
        if os.path.exists(date_file):
            os.system("mv " + date_file + " " + tdata_file)

    os.system("rm -rf " + output_directory)
    try:
        for t_date in dates:
            print(t_date)
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

        "Getting the tick_change so that we can scale up or down the model stdev based on tick change; (model_stdev*tick_change)"
        tick_change = get_tick_ratio_for_dates(shortcode, model_for_date, config_created_date)

        # In this case we don't want to learn weights by regression
        # Instead just normalize the weights to have constant stdev across different set of dates
        if regress_exec == "CONSTANT_REBALANCING":
            # the final model is same as the init model
            processed_model_file = ilist
            process_model(model_process_string, dates_file, start_time, end_time,
                          processed_model_file, ilist, reg_string, tick_change)
            # sign check is passed since the weights are given by the user manually
            if parent_work_dir is not None:
                work_dir = None
            return True, work_dir, processed_model_file

        out = run_method(ilist, filtered_file, reg_string, reg_output_file, final_model_file, config_json)

        # If the regress exec doesn't create a model file, dont throw error, let the modelling process continue
        if not os.path.exists(final_model_file):
            return (True, work_dir, final_model_file)

        try:
            open(final_model_file)
        except IOError:
            print(final_model_file + " not readable !")
            raise ValueError(
                final_model_file + " failed to create model file, please add code to catch error before coming here")

        # finally lets adjust stdev of model to specified value
        # if model_process_string == 0, do nothing
        # if model_process_string == 1, adjust init_model_file Stdev ( should be part of INDICATOREND # Corr: __value__ StDev: __value__ RS: __value__ ). we could init_model_file stdev for current period, but we are more interested in tying it some fixed value
        processed_model_file = final_model_file

        process_model(model_process_string, dates_file, start_time, end_time,
                      processed_model_file, ilist, reg_string, tick_change)
        sign_check_passed = True
        if sign_check_string == "1":
            sign_check_passed = check_sign_with_ilist(ilist, processed_model_file)
        # if model_process_string == 2, adjust to dep stdev
        # get_stdev_model

        # check with DB about this
        # basic validation of model tests can include
        # mean, stdev, min_corr with dep, min_R^2, min_no_of_zero_crossings_per_day, fat_tail_check (values > 75% / values < 75%)
        # check = check_correlations(processed_model_file, shortcode, t_date, num_days, indicator_sharpe_threshold, model_sharpe_threshold)
        # if check:
        #    os.system("cp " + processed_model_file + " " + ilist + model_for_date)
        # else:
        #    print "Check Failed, could be overfitting"
        if parent_work_dir != None:
            work_dir = None

        return (sign_check_passed, work_dir, processed_model_file)
    except Exception as e:
        print("There was error while building the model : ")
        print(str(e))
        print("\n")
        if os.path.exists(work_dir):
            shutil.rmtree(work_dir)
            sys.exit()
