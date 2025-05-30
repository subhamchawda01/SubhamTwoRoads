#!/usr/bin/env python

# this script is used to get list of configs for a pool
# pool is determined by shortcode, start_time, end_time
# that can be provided explicitly or fetched from a config


import os
import sys
import argparse

sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.wf_db_utils.pool_utils import get_configs_for_pool
from walkforward.wf_db_utils.pool_utils import get_pool_configs_for_config


def get_configs_list(args):
    if args.input_mode == 'CONFIG':
        configlist = get_pool_configs_for_config(args.configname, args.normal_or_staged)

    elif args.input_mode == 'POOL':
        if args.timeperiod and args.st is None and args.et is None:
            tp_tokens = args.timeperiod.split('-')
            assert len(tp_tokens) >= 2
            st, et = tp_tokens[0:2]
            if len(tp_tokens) > 2:
                args.pt = tp_tokens[2]
        else:
            st, et = args.st, args.et

        if args.evtok != '':
            configlist = get_configs_for_pool(args.shc, st, et, args.normal_or_staged, args.pt, 'EBT', args.evtok)
        else:
            configlist = get_configs_for_pool(args.shc, st, et, args.normal_or_staged, args.pt)

    else:
        print("Mode not recognized..")
        return
    print("\n".join(configlist))


parser = argparse.ArgumentParser()
parser.add_argument('-c', dest='configname', help="the walk-forward-config identifier", type=str)
parser.add_argument('-m', dest='input_mode', help="input_mode CONFIG/POOL", type=str, required=True)
parser.add_argument('-shc', dest='shc', help='shortcode', type=str)
parser.add_argument('-st', dest='st', default=None, help='start_time', type=str)
parser.add_argument('-et', dest='et', default=None, help='end_time', type=str)
parser.add_argument('-pooltag', dest='pt', help='pool_tag', default="", type=str, required=False)
parser.add_argument('-tp', dest='timeperiod',
                    help='timeperiod (either provide st,et or provide timeperiod)', default="", type=str)
parser.add_argument('-type', dest='normal_or_staged',
                    help='S for staged / N for pool (default: N)', default='N', type=str)
parser.add_argument('-evtok', dest='evtok', help='event_token for EBT strat', default='', type=str)

args = parser.parse_args()
assert args.normal_or_staged in ['S', 'N']
assert args.input_mode in ['CONFIG', 'POOL']

get_configs_list(args)
