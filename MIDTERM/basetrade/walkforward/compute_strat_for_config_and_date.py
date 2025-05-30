#!/usr/bin/env python

"""
Arguments : <configname> <tradingdate>
"""

import os
import sys
import json
import subprocess
import shutil

from walkforward.wf_db_utils.fetch_config_details import fetch_config_details
from walkforward.wf_db_utils.fetch_strat_from_config_and_date import fetch_strat_from_config_and_date

from walkforward.wf_db_utils import dump_strat_for_config_for_day

from walkforward.definitions import execs
from walkforward.utils.date_utils import calc_prev_week_day

from walkforward.choose_best_param_for_model import choose_best_param_for_model
from walkforward.choose_model_param_for_regime import choose_model_param_for_regime

# type6
from walkforward.refresh_model import refresh_model
from walkforward.refresh_model_for_di import refresh_model_for_di


def compute_strat_for_config_and_date(configname, tradingdate, lookback, overwrite=False, parent_work_dir=None):

    this_cfg = fetch_config_details(configname)
    configname = os.path.basename(configname)

    if not this_cfg.is_valid_config():
        print("Invalid Config " + configname, file=sys.stderr)
        this_cfg.print_config(True)
        return

    config_json = json.loads(this_cfg.config_json)
    (model, param) = ("INVALID", "INVALID")

    day_to_model_param_pair = {}

    cfg_type = this_cfg.config_type
    if cfg_type == 3:
        # this is just a placeholder for existing strategy, noting needs to be done here
        tradingdate = 19700101
        if 'model_list' in config_json and 'param_list' in config_json:
            model = config_json['model_list'][0]
            param = config_json['param_list'][0]

        (shortcode, execlogic, modelfilename, paramfilename, start_time, end_time, strat_type,
         event_token, query_id) = fetch_strat_from_config_and_date(configname, tradingdate, 1)
        print((modelfilename + " " + paramfilename))
        if overwrite or modelfilename == 'INVALID' or paramfilename == 'INVALID':
            day_to_model_param_pair[tradingdate] = (model, param)
        else:
            print("not doing anything as of now")
    elif cfg_type == 4:
        # this is where we compute the param each day with given config
        choose_best_param_for_model(this_cfg, configname, tradingdate, lookback,
                                    day_to_model_param_pair, overwrite)
    elif cfg_type == 5:
        # this is where we choose the model for today based on recent days features
        choose_model_param_for_regime(this_cfg, configname, tradingdate, lookback,
                                      day_to_model_param_pair, overwrite)
    elif cfg_type == 2:
        # given a model, find optimal weights ( fbmfs)
        print("FBMFS")
    elif cfg_type == 1:
        # compute a fresh model from given parameters in config
        print("MODELWF")
    elif cfg_type == 6:
        # refresh model
        # configname: nothing changes except for model
        # tradingdate: most likely, for tomorrow
        # lookback: for how many days we need to refresh this model
        refresh_model(configname, tradingdate, lookback, day_to_model_param_pair, parent_work_dir, overwrite)

    elif cfg_type == 7:
        refresh_model_for_di(configname, tradingdate, lookback, day_to_model_param_pair, parent_work_dir, overwrite)

    else:
        print("config type not valid for " + configname, file=sys.stderr)

    for key in sorted(day_to_model_param_pair.keys()):
        (model, param) = day_to_model_param_pair[key]
        if model != "INVALID" and param != "INVALID":
            print(("For DATE: " + str(key) + " And CONFIG: " + configname +
                   "\nMODEL : " + model + "\nPARAM: " + param))
            if cfg_type != 6:
                dump_strat_for_config_for_day.dump_strat_for_config_for_day(model, param, key, configname, overwrite)
