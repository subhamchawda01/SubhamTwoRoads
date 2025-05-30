#!/usr/bin/env python
"""
Utility to get config to date to pnl map
"""

import os
import sys
import pandas as pd
import numpy as np
from datetime import datetime

import shutil

sys.path.append(os.path.expanduser('~/basetrade/'))
from walkforward.definitions import execs
from walkforward.utils import date_utils
from walkforward.utils.run_exec import exec_function


def load_config_date_pnl_map(shc, summarize_pnl_pool, start_date, end_date, pool_tag):

    config_date_pnl = {}
    summarize_pnl_command = [execs.execs().summarize_strategy, shc, summarize_pnl_pool, "DB", str(start_date),
                             str(end_date), "IF", "kCNAPnlSharpe", "0", "IF", "0", pool_tag]
    summarize_pnl_command = " ".join(summarize_pnl_command)
    PnL_data = exec_function(summarize_pnl_command)[0].strip()

    # fill the map of config_date_pnl from output of summarize strategy results
    PnL_data = PnL_data.split("\n")
    for line in PnL_data:
        if line == "":
            continue
        words = line.split(" ")
        if words[0] == "STRATEGYFILEBASE":
            config = words[1]
            config_date_pnl[config] = {}
            continue
        if words[0] == "STATISTICS":
            continue
        date_ = int(words[0])
        config_date_pnl[config][date_] = int(words[1])

    return config_date_pnl


def get_config_uts_n_date_pnl(shc, summarize_pnl_pool, start_date, end_date, pool_tag):
    config_uts_n_date_pnl = {}
    summarize_pnl_command = [execs.execs().summarize_strategy, shc, summarize_pnl_pool, "DB", str(start_date),
                             str(end_date), "IF", "kCNAPnlSharpe", "0", "IF", "0", pool_tag]
    summarize_pnl_command = " ".join(summarize_pnl_command)
    PnL_data = exec_function(summarize_pnl_command)[0].strip()

    # fill the map of config_date_pnl from output of summarize strategy results
    PnL_data = PnL_data.split("\n")

    for line in PnL_data:
        if "no strats found" in line or "#" in line:
            return config_uts_n_date_pnl
        if line == "":
            continue
        words = line.split(" ")
        if words[0] == "STRATEGYFILEBASE":
            config = words[1]
            config_uts_n_date_pnl[config] = {}
            continue
        if words[0] == "STATISTICS":
            sim_uts = words[26]
            config_uts_n_date_pnl[config]["sim_uts"] = int(float(sim_uts))
            continue

        date_ = int(words[0])
        config_uts_n_date_pnl[config][date_] = int(words[1])

    return config_uts_n_date_pnl
