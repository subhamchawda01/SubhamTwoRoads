#!/usr/bin/env python

import os
import sys
import argparse
import getpass
import time
import numpy as np

sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.definitions import execs
from walkforward.wf_db_utils.fetch_strat_from_config_struct_and_date import fetch_strat_from_config_struct_and_date
from walkforward.wf_db_utils.fetch_strat_from_config_and_date import fetch_strat_from_config_and_date

from pylib.pnl_modelling_utils.generate_pnl_stats import get_covariance_matrix
from pylib.pnl_modelling_utils.weights_util import get_model_stdev


def get_weights_from_model(model_file):
    """

    :param model_file : str
    :return: weights  : list
    reads the model file and returns the list of model weights
    """

    ILIST = open(model_file, "r")
    lines = ILIST.read().splitlines()

    weights = []
    for line in lines:
        indx = line.find("#")
        if indx != -1:
            line = line[:indx]
        words_list = line.strip().split()
        # in case of blank line
        if len(words_list) == 0:
            continue
        if words_list[0] == "INDICATOR":
            weights.append(float(words_list[1]))

    return weights


# generate the parser and add the arguments
parser = argparse.ArgumentParser()

parser.add_argument("--shortcode", "-s", help="shortcode", required=False)
parser.add_argument("--model_file", "-m", help="model for which to get the relative weights", required=False)
parser.add_argument("--start_date", "-sd", help="start date", required=True)
parser.add_argument("--end_date", "-ed", help="end date", required=True)
parser.add_argument("--start_time", "-st", help="start time", required=False)
parser.add_argument("--end_time", "-et", help="end time", required=False)
parser.add_argument("--config", "-c", help="config", required=False)

args = parser.parse_args()


# parse the arguments
shortcode = args.shortcode
model_file = args.model_file

config = args.config
start_date = args.start_date
end_date = args.end_date

# create the working directory
work_dir = "/media/shared/ephemeral16/" + getpass.getuser() + "/ind_stats/" + "/" + \
           str(int(time.time() * 1000)) + "/"

os.system("rm -rf " + work_dir)
os.system("mkdir -p " + work_dir)

if config != None:
    strat_file = work_dir + "strat"

    if type(config) == str:
        (shortcode, execlogic, model_file, param_file, start_hr, end_hr, strat_type, event_token,
                  query_id) = fetch_strat_from_config_and_date(config, end_date, 15)
    else:
        (shortcode, execlogic, model_file, param_file, start_hr, end_hr, strat_type, event_token,
         query_id) = fetch_strat_from_config_struct_and_date(config, end_date, 15)
    if model_file == "IF" or param_file == "IF":
        print("Error in fetch_strat for " , config)
        sys.exit()

    filehandle = open(strat_file, "w")

    filehandle.write("STRATEGYLINE" + " " + shortcode + " " + execlogic + " " + model_file+ " " + param_file
                  + " " + start_hr + " " + end_hr + " " + str(query_id) + " " + event_token)
    filehandle.close()

if args.start_time != None:
    start_hr = args.start_time

if args.end_time != None:
    end_hr = args.end_time

# get the weights from model file
weights = get_weights_from_model(model_file)


# call get_ind_stats_for_ilist to the stdevs of each indicator
cmd_to_generate_data = [execs.execs().get_ind_stats_for_ilist, shortcode, model_file,
                        start_date, end_date, start_hr, end_hr, work_dir]
cmd_to_generate_data = " ".join(cmd_to_generate_data)

os.system(cmd_to_generate_data)

# get the weights from the pnl_stats file
stats_file = work_dir + "/IndicatorStats/pnl_stats"
log_file = work_dir + "/main_log_file.txt"


stdevs, covariance_matrix = get_covariance_matrix(stats_file, log_file)

# multiply model weight with stdev and divide it by the minimum of the model_weight*stdev to get the relative weights
relative_weights = []
for i in range(len(stdevs)):
    relative_weights.append(abs(weights[i] * stdevs[i]))

np_weights = np.array(weights)
np_relative_weights = np.array(relative_weights)

print("\n\nRelative weights are :")
sum = 0
for i in range(len(stdevs)):
    # print(round(np_relative_weights[i]/np_relative_weights.min()*np.sign(weights[i]),0))
    sum += abs(round(np_relative_weights[i] / np_relative_weights.min() * np.sign(weights[i]), 0))

for i in range(len(stdevs)):
    print(int(round(np_relative_weights[i] / np_relative_weights.min() * np.sign(weights[i]) / sum * 10, 0)))

model_stdev = get_model_stdev(np_weights, covariance_matrix)

print("Model Stdev is : " + str(model_stdev))
