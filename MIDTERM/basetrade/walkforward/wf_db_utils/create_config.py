#!/usr/bin/env python


"""

Creates the config json from the command line arguments

"""


import argparse
import subprocess
import sys
import os
import time
import signal

parser = argparse.ArgumentParser(description="creates a new config in DB")

# config type 3
# required fields
# configid, shortcode, execlogic, start_time, end_time, strat_type, query_id, config_type, config_json, model_list, param_list

strat_type = "Regular"

parser.add_argument('-shortcode', help='dependant shortcode', required=True, dest='shortcode')
parser.add_argument('-execlogic', help='PriceBasedAggressive/PriceBasedScalper ..', required=True, dest='exec_logic')
parser.add_argument('-trading_stime', help='start time for trading', required=True, dest='start_time')
parser.add_argument('-trading_etime', help='end time for trading', required=True, dest='end_time')
parser.add_argument('-strat_type', help='EBT/Regular', required=False, dest='strat_type')

# DB handles this to make it uniq
#parser.add_argument('-query_id', help='some uniqid', required=False, dest='query_id')

parser.add_argument('-config_type', help='3 - 6 are supported', type=int, required=True, dest='config_type')
parser.add_argument('-model_file', help='full model file path name', required=True, dest='model_file')
parser.add_argument('-param_file', help='end time to create data', required=True, dest='param_file')
parser.add_argument('-config_name', help='name for the config', required=False, dest='config_name')

# additional for type 6
parser.add_argument('-ddays_string', help='smart string to prepare dates to be used for datagen ', dest='ddays_string')
parser.add_argument('-trigger_string', help='somestring to identify trigger logic', dest='trigger_string')
parser.add_argument(
    '-td_string', help='time data prep string, give 3 args (time, events, trade cutoffs) space seperated', nargs=3, dest='td_string')
parser.add_argument(
    '-rd_string', help='red data prep string, give 2 args ( process_algo, duration in secs ) space seperated', nargs=2, dest='rd_string')
parser.add_argument('-rdata_process_string', help='filters to be applied on rdata', dest='rdata_process_string')
parser.add_argument(
    '-reg_string', help='regression string as you would specify in gen_strat instruction file', dest='reg_string')
parser.add_argument('-model_process_string',
                    help='specify integers, 0 = change nothing, 1 = adjust to dep_stdev, 2 = adjust to specific stdev', dest='model_process_string')

args = parser.parse_args()
param_list = []
param_list.append(args.param_file)

model_list = []
model_list.append(args.model_file)

if args.config_name is None:
    base_model_name = args.model_file.split("/")[-1]
    base_model_name_elements = base_model_name.split("_")
    if len(base_model_name_elements) > 5:
        del base_model_name_elements[0]
        del base_model_name_elements[0]
    base_config_name = "_".join(base_model_name_elements)
    base_param_name = args.param_file.split("/")[-1]
    args.config_name = base_config_name + base_param_name.split("_")[-1]

if args.strat_type is None:
    args.strat_type = "Regular"

#ddays_string = None
#trigger_string = None
#td_string = None
#rd_String = None
#rdata_process_string = None
#reg_string = None
#model_process_string = None


if args.config_type is 6:
    if args.ddays_string is None:
        raise ValueError("ddays_string is required for type 6 config")
    if args.trigger_string is None:
        raise ValueError("trigger_string is required for type 6 config")
    if args.td_string is None:
        raise ValueError("td_string is required for type 6 config")
        print()
    if args.rd_string is None:
        raise ValueError("rd_string is required for type 6 config")
        print()
    if args.rdata_process_string is None:
        raise ValueError("rdata_process_string is required for type 6 config")
        print()
    if args.reg_string is None:
        raise ValueError("reg_string is required for type 6 config")
        print()
    if args.model_process_string is None:
        raise ValueError("model_process_string is required for type 6 config")
    else:
        strat_config = {"config_type": args.config_type,
                        "shortcode": args.shortcode,
                        "execlogic": args.exec_logic,
                        "model_list": model_list,
                        "param_list": param_list,
                        "start_time": args.start_time,
                        "end_time": args.end_time,
                        #"query_id" : query_id,
                        "strat_type": args.strat_type,
                        "ddays_string": args.ddays_string,
                        "trigger_string": args.trigger_string,
                        "td_string": args.td_string,
                        "rd_string": args.rd_string,
                        "rdata_process_string": args.rdata_process_string,
                        "reg_string": args.reg_string,
                        "model_process_string": args.model_process_string}
elif args.config_type is 3:
    strat_config = {"config_type": args.config_type,
                    "shortcode": args.shortcode,
                    "execlogic": args.exec_logic,
                    "model_list": model_list,
                    "param_list": param_list,
                    "start_time": args.start_time,
                    "end_time": args.end_time,
                    #"query_id" : query_id,
                    "strat_type": args.strat_type}
else:
    raise NotImplementedError("Author is lazy to implement rest of configs, only supports 3 and 6")
    #raise NotImplementedError


# config type 6
# config type 3 + config_type, execlogic, shortcode, model_list, param_list, start_time, end_time, strat_type, event_token, queryid
# trigger_string, lookback_days, td_string, rd_string, preprocess_string, reg_string, postprocess_string
# model in model_list will be used as ilist !
