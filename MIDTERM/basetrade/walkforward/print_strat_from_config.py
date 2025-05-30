#!/usr/bin/env python
from __future__ import print_function

import os
import sys
import argparse

sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.wf_db_utils.fetch_strat_from_config_and_date import fetch_strat_from_config_and_date
from walkforward.wf_db_utils.fetch_structured_strat_from_config_struct_and_date import fetch_structured_strat_from_name_and_date

from walkforward.wf_db_utils.db_handles import set_backtest


def print_strat_from_config(arg_input):
    """
    Just a print function
    :param arg_input: 
    :return: 
    """
    if arg_input.use_backtest == 1 or 'USE_BACKTEST' in os.environ:
        set_backtest(True)
        os.environ['USE_BACKTEST'] = "1"

    if not arg_input.is_structured:
        (shortcode, execlogic, modelname, paramname, start_time, end_time, strat_type, event_token,
        query_id) = fetch_strat_from_config_and_date(arg_input.configname, arg_input.tradingdate, arg_input.lookback_days)

        strategyline_prefix = "STRATEGYLINE"
        if strat_type == "MRT":
            strategyline_prefix = "PORT_STRATEGYLINE"

        if modelname != "IF" and paramname != "IF":
            print(strategyline_prefix + " " + shortcode + " " + execlogic + " " + modelname + " " + paramname
                  + " " + start_time + " " + end_time + " " + str(query_id) + " " + event_token)
        else:
            print("No Entry for config in database", file=sys.stderr)
    else:
        print(fetch_structured_strat_from_name_and_date(arg_input.configname, arg_input.tradingdate, arg_input.lookback_days))


parser = argparse.ArgumentParser()
parser.add_argument('-c', dest='configname', help="the walk-forward-config identifier", type=str)

parser.add_argument('-p', dest='wf_config_path',
                    help="the walk-forward-config directory path in case we are not getting it from the database",
                    type=str, default='/home/dvctrader/modelling/', required=False)

parser.add_argument('-d', dest='tradingdate',
                    help="the date on which we are looking to trade with this walk-forward config", type=str)

parser.add_argument('-b', dest='use_backtest', help='Weather to fetch from backtest database', type=int, default=0)
parser.add_argument('-l', dest='lookback_days', help='lookback days for strategy for given config in case strat for '
                                                     'given date is not available', type=int, default=15)
parser.add_argument('-s', dest='is_structured', help='Is This structured config', type=int, default=0)

args = parser.parse_args()

print_strat_from_config(args)
