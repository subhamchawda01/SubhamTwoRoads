#!/usr/bin/env python


"""
This module takes input as conifig and tradingdate
and tries to choose a param/model based on recent days
avg feature value
Assumption that no. of regimes should match with either
 a) No. of models in modelist
 b) No. of params in paramlist
or both

And the regime cutoff are given in ascending order

 Input:
 json string should have following things
 a) sort_algo
 b) max_ttc

"""

import os
import json
import subprocess

from walkforward.definitions import execs
from walkforward.utils.date_utils import calc_prev_week_day

from walkforward.wf_db_utils.fetch_dump_model import fetch_modelid_from_modelname
from walkforward.wf_db_utils.fetch_dump_param import fetch_paramid_from_paramname
from walkforward.wf_db_utils.fetch_config_details import fetch_config_id

from walkforward.wf_db_utils.get_trading_days_for_shortcode import get_trading_days_for_shortcode

from walkforward.wf_db_utils.get_optimized_days_for_which_model_param_is_missing import \
    get_optimized_days_for_which_model_param_is_missing


def choose_model_param_for_regime(cfg, configname, tradingdate, num_days, day_to_model_param_pair, overwrite):
    # print "Call: choose_best_param_for_model: " + cfg.config_json + str(tradingdate)
    config_json = json.loads(cfg.config_json)

    # here we are assuming one and one relationship with model_list and model record in DB
    # however I think we should stop referring to model_list/param_list, instead use DB
    # in case we dont have model records and param records we should add them as well

    config_id = fetch_config_id(configname)

    modelfilelist = config_json['model_list']
    for model in modelfilelist:
        model_id = fetch_modelid_from_modelname(model, True, config_id)
        print(model_id)

    paramfilelist = config_json['param_list']
    for param in paramfilelist:
        param_id = fetch_paramid_from_paramname(param, True, config_id)
        print(param_id)

    modelfilename = ""
    paramfilename = ""

    if 'feature_lookback_days' in config_json:
        lookback_days = config_json['feature_lookback_days']
    else:
        print("choose_model_param_for_regime: Feature look back days not found. Taking 5 as its value")
        lookback_days = 5

    date_pair_vec_to_run_sim = []
    no_results_day_vec = []
    tr_start_date = calc_prev_week_day(tradingdate, num_days)
    if not overwrite:
        (date_pair_vec_to_run_sim, no_results_day_vec) = get_optimized_days_for_which_model_param_is_missing(cfg, configname,
                                                                                                             num_days, tradingdate,
                                                                                                             lookback_days)
    else:
        no_results_day_vec = get_trading_days_for_shortcode(cfg.shortcode, 0, tradingdate, num_days)
        date_pair_vec_to_run_sim.append((tr_start_date, tradingdate))

    invalid_config = False

    if 'sample_feature' in config_json:
        feature_string = config_json['sample_feature']
        if len(feature_string) < 2:
            print("choose_model_param_for_regime: Incorrect Feature string."
                  "using INVALID model and param")
            invalid_config = True
    else:
        print("choose_model_param_for_regime: Feature string not found."
              "using INVALID model and param")
        invalid_config = True

    if 'feature_switch_threshold' in config_json:
        switch_threshold = config_json['feature_switch_threshold']
        switch_threshold = list(map(float, switch_threshold))
    else:
        print("choose_model_param_for_regime: Switch thresholds not found."
              "using INVALID model and param")
        modelfilename = "INVALID"
        paramfilename = "INVALID"
        invalid_config = True

    num_regime = len(switch_threshold) + 1
    # checking if the cutoff are in ascending order
    if num_regime > 2:
        for i in range(0, num_regime - 2):
            if switch_threshold[i] > switch_threshold[i + 1]:
                print("choose_model_param_for_regime: Regime cutoffs not specified in ascending order."
                      "using INVALID model and param")
                modelfilename = "INVALID"
                paramfilename = "INVALID"
                invalid_config = True

    global_model_index_to_use = -1
    global_param_index_to_use = -1

    # checking the model to use
    if len(modelfilelist) > 1:
        if len(modelfilelist) != num_regime:
            print("No. of models doesn't match with no. of regimes. INVALID model")
            modelfilename = "INVALID"
    elif len(modelfilelist) == 1:
        global_model_index_to_use = 0
    else:
        print("choose_model_param_for_regime: No model file in the model_list")
        modelfilename = "INVALID"

    # checking the param to use
    if len(paramfilelist) > 1:
        if (len(paramfilelist) != num_regime):
            print("No. of params doesn't match with no. of regimes. INVALID param")
            paramfilename = "INVALID"
    elif len(paramfilelist) == 1:
        global_param_index_to_use = 0
    else:
        print("choose_model_param_for_regime: No param file in the param_list")
        paramfilename = "INVALID"

    if invalid_config or modelfilename == "INVALID" or paramfilename == "INVALID":
        day_to_model_param_pair[tradingdate] = (modelfilename, paramfilename)
        return

    feature_shortcode = str(feature_string[0])
    feature = ' '.join([str(x) for x in feature_string[1:]])

    model_index_to_use = global_model_index_to_use
    param_index_to_use = global_param_index_to_use

    for date in no_results_day_vec:

        avg_feature_val_cmd = [execs.execs().avg_samples, feature_shortcode, str(calc_prev_week_day(date, 1)), str(lookback_days), cfg.start_time,
                               cfg.end_time, '0', feature]

        avg_feature_out = subprocess.Popen(' '.join(avg_feature_val_cmd), shell=True, stdout=subprocess.PIPE)
        avg_feature_val = float(avg_feature_out.communicate()[0].decode('utf-8').strip().split()[-1])

        if avg_feature_val > 0:
            for i in range(0, num_regime - 1):
                if avg_feature_val < switch_threshold[i]:
                    if global_model_index_to_use == -1:
                        model_index_to_use = i
                    if global_param_index_to_use == -1:
                        param_index_to_use = i
                    break
                elif avg_feature_val >= switch_threshold[i]:
                    if global_model_index_to_use == -1:
                        model_index_to_use = i + 1
                    if global_param_index_to_use == -1:
                        param_index_to_use = i + 1
        else:
            print("Sample data is not present or Feature name is incorrect. Choosing first model and param")
            model_index_to_use = 0
            param_index_to_use = 0

        modelfilename = modelfilelist[model_index_to_use]
        paramfilename = paramfilelist[param_index_to_use]

        if not os.path.exists(modelfilename):
            print("choose_model_for_regime: " + modelfilename + "does not exist. Exiting")
            modelfilename = "INVALID"

        if not os.path.exists(paramfilename):
            print("choose_model_for_regime: " + paramfilename + "does not exist. Exiting")
            paramfilename = "INVALID"

        day_to_model_param_pair[date] = (modelfilename, paramfilename)
