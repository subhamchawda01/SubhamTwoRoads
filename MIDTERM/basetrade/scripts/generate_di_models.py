#!/usr/bin/python

from datetime import date
import sys
import os
import subprocess
import math
import operator
import heapq
import argparse
import getpass
import time

sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.definitions import execs
from scripts.get_di_universe import GetDV01Volume
from scripts.get_di_universe import GetShortCodeList
from scripts.get_di_sources import GetSources
from scripts.get_di_ilist import MakeIlist
from walkforward.utils.prepare_rdata import convert_t2d
from walkforward.utils.process_rdata import apply_filters
from walkforward.utils.prepare_tdata import run_datagen
from walkforward.utils.learn_signal_weights import run_method, run_fill_based_model_generator
from walkforward.wf_db_utils.utils import FileExistsWithSize


def NumColumsRdataFile(rdata_file):
    ncols = os.popen("cat " + rdata_file + " | head -n1 | grep -o \" \" | wc -l").read()
    return ncols


def NumRowsIlist(ilist_file):
    nrows = os.popen("cat " + ilist_file + " | grep INDICATOR | grep 1 | wc -l").read()
    return nrows


def build_model(shortcode, ilist, date, num_days, start_time, end_time, ptime, events, trades, pred_duration, pred_algo, data_filter, reg_string, work_dir, is_ilist_changed, config_id, config_json):
    t0 = time.time()
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

    if not os.path.isdir(work_dir):
        os.system("mkdir " + work_dir)
    if not (os.path.isdir(work_dir + shortcode + "/")):
        os.system("mkdir " + work_dir + shortcode)

    catted_file = work_dir + shortcode + "/" + "catted_file_"
    filtered_file = work_dir + shortcode + "/" + "regdata_file_"
    final_model_file = work_dir + date + "/" + shortcode + "/" + \
        "finalmodel_file_date_" + date + "_conifgid_" + str(config_id)
    reg_output_file = work_dir + shortcode + "/" + "reg_output_file"

    if os.path.isfile(catted_file):
        os.system("rm -r " + catted_file)
    if os.path.isfile(filtered_file):
        os.system("rm -r " + filtered_file)
    if os.path.isfile(reg_output_file):
        os.system("rm -r " + reg_output_file)

    for t_date in dates:
        tdata_file = work_dir + shortcode + "/" + "tdata_file_" + t_date
        rdata_file = work_dir + shortcode + "/" + "rdata_file_" + t_date

        if FileExistsWithSize(rdata_file) and is_ilist_changed == False and NumRowsIlist(ilist) == NumColumsRdataFile(rdata_file):
            print(("rdata file already exists for this date " + t_date))
            os.system("cat " + rdata_file + " >> " + catted_file)
            continue
        else:
            print("Regenerating Regdata")
        out, err, errcode = run_datagen(ilist, t_date, start_time, end_time, tdata_file, ptime, events, trades)
        if len(err) > 0:
            print("Error: reported in datagen module", err)
        if errcode != 0:
            print("Error: received non zero exitcode from datagen module", errcode)

        # filter is sent if we have to/can apply it while converting
        out, err, errcode = convert_t2d(shortcode, t_date, ilist, tdata_file, pred_duration, pred_algo,
                                        rdata_file, data_filter)

        if len(err) > 0:
            print("Error: reported in t2d module", err)
        if errcode != 0:
            print("Error: received non zero exitcode fro, t2d module", errcode)

        os.system("cat " + rdata_file + " >> " + catted_file)
    # end of data loop
    out, err, errcode = apply_filters(shortcode, catted_file, data_filter, filtered_file)

    if len(err) > 0:
        print("Error: reported in filters module", err)
    if errcode != 0:
        print("Error: received non zero exitcode from, filters module", errcode)

    # Config_Json is irrelevant in  this case so initializing to null
    print(filtered_file)
    print(reg_output_file)
    print(final_model_file)
    out = run_method(ilist, filtered_file, reg_string, reg_output_file, final_model_file, config_json)

    # If the regress exec doesn't create a model file, dont throw error, let the modelling process continue
    if not os.path.exists(final_model_file):
        return (True, work_dir, final_model_file)
    os.system("rm -r " + catted_file)
    os.system("rm -r " + filtered_file)
    os.system("rm -r " + reg_output_file)

    try:
        open(final_model_file)
    except IOError:
        print(final_model_file + " not readable !")
        raise ValueError(
            final_model_file + " failed to create model file, please add code to catch error before coming here")

    t1 = time.time()
    print("Time requied for building model for " + shortcode + " : " + str(t1 - t0))
    return final_model_file
