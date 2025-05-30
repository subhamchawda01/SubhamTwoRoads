#!/usr/bin/env python
# \file ModelScripts/refresh_model.py
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#


# INITARGS
# SDate LBackdays
# STime ETime
# EcoString ?
# RegimeString ?

# TIMESERIESDATA
# TD@Time,Events,Trades

# REGDATA
# RD@ProcessAlgo,PredDurationInSecs

# PREPROCESSING
# PRE@filter,scale

# WEIGHTSMETHOD
# LM FSRR SIGLR

# POSTPROCESSING
# SCALE


import argparse
import subprocess
import sys
import os
import time
import signal

sys.path.append(os.path.expanduser('~/basetrade/'))
from walkforward.definitions import execs
from walkforward.utils.prepare_tdata import run_datagen


def signal_handler(signal, frame):
    print("There is a siginit")
    sys.exit(0)


signal.signal(signal.SIGINT, signal_handler)


parser = argparse.ArgumentParser(description="refreshes existing model with new weights and/or rescales stdev")

# global params

# InitArgs
parser.add_argument('-mfile', help='inital model file', required=True, dest='model_file')

# TODO call smart_date script
# SD AvgPrice NA 0 16
# SD STDEV NA 0.2 0.3

parser.add_argument('-edate', help='latest date to create data', required=True, dest='end_date')
parser.add_argument('-lback', help='lookback days to create data', required=True, dest='lback_days')

# TODO
parser.add_argument('-stime', help='start time to create data', required=True, dest='start_time')
parser.add_argument('-etime', help='end time to create data', required=True, dest='end_time')

# Modules Args
parser.add_argument('-TDString', help='time-data-prep string includes time, events, trade to define print frequency',
                    nargs=3, required=True, dest='td_string')  # 3(time, events, trades)
parser.add_argument('-RDString', help='reg-data-prep string includes process algo and pred duration in seconds',
                    nargs=2, required=True, dest='rd_string')  # 2(process_algo, pred_duration)
parser.add_argument('-PrPString', help='include list of filters to apply or scaling algorithms to apply on the data',
                    dest='pre_processor')  # n(filter1, filter2, scale1, scale2)
# x(RIDGE 8 0.9, RIDGE 10 0.95, FSRR 0.75 0.04 0 0 0.95 20 N, FSRMFSS 6 0.04 0 0 0.6 20 N, FSRMSH 6 0.04 0 0 0.6 20 N)
parser.add_argument('-MString', help='regression inputs / pnl based method inputs, ex: RIDGE 10 0.95',
                    required=True, dest='method_string')
parser.add_argument('-PoPString', help='post processing (2/1/0), scale stdev of the model to dep-stdev or prev-stdev of the model or keep as it is',
                    dest='post_processor')  # 1(1=new_stdev, 2=scale_to_dep, 3=scale_to_prev)


# Step1:
# use shortcode, start and end date and get list of dates
# /home/kputta/basetrade/scripts/get_list_of_dates_for_shortcode.pl
# then in a loop, prepare reg_data ( remember to call run_t2d with output file in append mode )
# call filters functions
# call regresssion function
# adjust weights


args = parser.parse_args()

# iterate
out, err, errcode = run_datagen(args.model_file, args.end_date, args.start_time, args.end_time,
                                "/home/kputta/hello_there", args.td_string[0], args.td_string[1], args.td_string[2])

if len(err) > 0:
    print "Error: reported in datagen module", err
if errcode != 0:
    print "Error: received non zero exitcode", errcode

out, err, errcode = convert_t2d()
if len(err) > 0:
    print "Error: reported in datagen module", err
if errcode != 0:
    print "Error: received non zero exitcode", errcode


# import walkforward.utils.prepare_rdata import convert_t2d
# import walkforward.utils.process_rdata import apply_filters
# import walkforward.utils.learn_signal_weights import run_method
# import walkforward.utils.process_model_file import tune_model
