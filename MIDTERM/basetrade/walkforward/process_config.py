#!/usr/bin/env python

# this script is used to make changes to a config in database/flatfiles
# will update the description more as we add the functionalities


import os
import sys
import argparse

sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.utils.process_config_utils import add_config_to_database
from walkforward.utils.process_config_utils import modify_config_field
from walkforward.utils.process_config_utils import print_config
from walkforward.utils.process_config_utils import print_model_from_config
from walkforward.utils.process_config_utils import print_param_from_config
from walkforward.utils.process_config_utils import prune_config
from walkforward.utils.process_config_utils import remove_strats
from walkforward.utils.process_config_utils import remove_results
from walkforward.utils.process_config_utils import move_to_pool
from walkforward.utils.process_config_utils import update_pooltag
from walkforward.utils.process_config_utils import update_expect0vol
from walkforward.utils.process_config_utils import reload_config

from walkforward.wf_db_utils.db_handles import set_backtest
# set_backtest(True)


def process_config(configname, args, params):
    if args.mode == 'ADD':
        add_config_to_database(configname, params)
    elif args.mode == 'RELOAD':
        reload_config(configname,params)
    elif args.mode == 'EDIT':
        modify_config_field(configname, params, args)
    elif args.mode == 'VIEW':
        print_config(configname, False)
    elif args.mode == 'VIEWALL':
        print_config(configname, True)
    elif args.mode == 'VIEWMODEL':
        print_model_from_config(configname)
    elif args.mode == 'VIEWPARAM':
        print_param_from_config(configname)
    elif args.mode == 'PRUNE':
        prune_config(configname)
    elif args.mode == 'REMOVE_STRATS':
        remove_strats(configname, args.start_date, args.end_date)
    elif args.mode == 'REMOVE_RESULTS':
        remove_results(configname, args.start_date, args.end_date)
    elif args.mode == 'MOVE_TO_POOL':
        move_to_pool(configname)
        if args.pooltag is not None:
            update_pooltag(configname, args.pooltag)
        if args.expect0vol is not None:
            update_expect0vol(configname, int(bool(args.expect0vol)))
    elif args.mode == 'UPDATE_POOLTAG':
        update_pooltag(configname, args.pooltag)
    elif args.mode == 'SET_EXPECT0VOL':
        update_expect0vol(configname, 1)
    elif args.mode == 'UNSET_EXPECT0VOL':
        update_expect0vol(configname, 0)
    else:
        print("Mode not recognized..")


parser = argparse.ArgumentParser()
parser.add_argument('-c', dest='configname', help="the walk-forward-config identifier", type=str)
parser.add_argument('-m', dest='mode',
                    help="mode ADD/RELOAD/VIEW/VIEWALL/VIEWMODEL/VIEWPARAM/EDIT/PRUNE/REMOVE_STRATS/"
                         "REMOVE_RESULTS/MOVE_TO_POOL", type=str,  default='/home/dvctrader/modelling/', required=True)
parser.add_argument('-p', dest='params', help='parameters for modifying', nargs='+', required=False)
parser.add_argument('-sd', dest='start_date', help='start_date (for results/strats removal)', type=int, required=False)
parser.add_argument('-ed', dest='end_date', help='end_date (for results/strats removal)', type=int, required=False)
parser.add_argument('-pooltag', dest='pooltag', help='pooltag', type=str, required=False)
parser.add_argument('-expect0vol', dest='expect0vol', help='expect0vol', type=int, required=False)
parser.add_argument('--keep-results', dest='keep_results',
                    help='whether to remove results are not (in case of edit config)', default=False,
                    required=False, action='store_true')
parser.add_argument('--keep-strats', dest='keep_strats',
                    help='whether to remove strats or not (in case of edit config)', default=False,
                    required=False, action='store_true')
parser.add_argument('--use-backtest', dest='use_backtest',
                    help='whether to use backtest DB or not', default=False)
parser.add_argument('-s', dest='is_structured', help='whether the config is structured', default=0, type=int)

args = parser.parse_args()
if args.use_backtest:
    set_backtest(True)
    os.environ['USE_BACKTEST'] = "1"
if args.mode == 'REMOVE_STRATS' or args.mode == 'REMOVE_RESULTS':
    if not args.start_date or not args.end_date:
        parser.error('For mode REMOVE_STRATS and REMOVE_RESULTS, start_date and end_date is required')

process_config(args.configname, args, args.params)
