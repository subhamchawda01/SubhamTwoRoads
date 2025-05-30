#!/usr/bin/env python

import argparse
import subprocess
import sys
import os

sys.path.append(os.path.expanduser('~/basetrade/'))
from walkforward.definitions import execs

parser = argparse.ArgumentParser(description="refreshes existing model with new weights and/or rescales stdev")

# global params

# InitArgs
parser.add_argument('-edate', help='latest date to create data', required=True, dest='end_date')
parser.add_argument('-lback', help='lookback days to create data', required=True, dest='lback_days')
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

parser.parse_args()


# import walkforward.utils.prepare_tdata import run_datagen
# import walkforward.utils.prepare_rdata import convert_t2d
# import walkforward.utils.process_rdata import apply_filters
# import walkforward.utils.learn_signal_weights import run_method
# import walkforward.utils.process_model_file import tune_model

#call(["ls", "-l"])

#  ~/basetrade_install/bin/datagen ~/ilist 20170323 EST_800 UTC_1900 2897 ~/kp_dout 1000 0 0 0
