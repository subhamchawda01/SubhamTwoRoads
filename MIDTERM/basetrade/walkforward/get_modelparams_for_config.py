#!/usr/bin/env python

"""
Utility to print/plot list of models params for given config for last n days
"""


import os
import sys
import datetime
import argparse

try:
    import matplotlib.pyplot as plt
except ImportError:
    print("Coult not import Matplotlib, Only listing is allowed", file=sys.stderr)


sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.utils.get_modelparams_for_config_util import get_modelparams_for_config


def show_modelparam(date_to_modelparam_pair, mode):
    if 'LIST' == mode:
        for date in sorted(date_to_modelparam_pair.keys()):
            print(("%d %s %s" % (date, date_to_modelparam_pair[date][0], date_to_modelparam_pair[date][1])))
    elif 'VIEW' == mode:
        model_vec = []
        param_vec = []
        model_idx_vec = []
        param_idx_vec = []

        date_vec = sorted(date_to_modelparam_pair.keys())
        days_from_base_vec = []
        if len(date_vec) > 0:
            smallest_day = datetime.datetime.strptime(str(date_vec[0]), '%Y%m%d')
            for day in date_vec:
                days_from_base_vec.append((datetime.datetime.strptime(str(day), "%Y%m%d")
                                           - smallest_day).total_seconds() / 86400)

        for date in date_vec:
            model_vec.append(date_to_modelparam_pair[date][0])
            param_vec.append(date_to_modelparam_pair[date][1])

        model_to_model_index = {}
        param_to_param_index = {}

        # create the index vectors
        for model in model_vec:
            if model not in list(model_to_model_index.keys()):
                model_to_model_index[model] = len(list(model_to_model_index.keys()))
            model_idx_vec.append(model_to_model_index[model])

        for param in param_vec:
            if param not in list(param_to_param_index.keys()):
                param_to_param_index[param] = len(list(param_to_param_index.keys()))
            param_idx_vec.append(param_to_param_index[param])

        for param in list(param_to_param_index.keys()):
            print("PARAM:", param_to_param_index[param], param)
        for model in list(model_to_model_index.keys()):
            print("MODEL:", model_to_model_index[model], model)

        plt.figure()
        plt.plot(days_from_base_vec, model_idx_vec, '-r')
        plt.plot(days_from_base_vec, param_idx_vec, '-b')
        plt.xlabel("Num Days From Start")
        plt.ylabel("Model/Param Index")
        plt.show()


parser = argparse.ArgumentParser()
parser.add_argument('-c', dest='configname', help="the walk-forward-config identifier", type=str, required=True)
parser.add_argument('-d', dest='tradingdate', help="date/end-date to generate the strat for", type=int, required=True)
parser.add_argument('-l', dest='lookback', help='look_back_days to generate the strat for', default=1, type=int)
parser.add_argument('-m', dest='mode', help='mode to view the params [LIST/VIEW]', default='LIST', type=str)


args = parser.parse_args()

date_to_model_param_pair_global = get_modelparams_for_config(args.configname, args.tradingdate, args.lookback)
show_modelparam(date_to_model_param_pair_global, args.mode)
