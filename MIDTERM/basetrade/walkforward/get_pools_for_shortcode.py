#!/usr/bin/env python

# this script is used to get list of configs for a pool
# pool is determined by shortcode, start_time, end_time
# that can be provided explicitly or fetched from a config


import os
import sys
import argparse

sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.utils.timeperiod_utils import is_pool_in_tp
from walkforward.wf_db_utils.pool_utils import get_pools_for_shortcode

parser = argparse.ArgumentParser()
parser.add_argument('-shc', dest='shc', help='shortcode', required=True, type=str)
parser.add_argument('-type', dest='normal_or_staged',
                    help='S for staged / N for pool (default: N)', default='N', type=str)
parser.add_argument('-tp', dest='timeperiod', help='pools inside the timeperiod', default='', type=str)
parser.add_argument('-ebt', dest='is_ebt', help='EBT pools (0/1)', default=0, type=int)

args = parser.parse_args()
assert args.normal_or_staged in ['S', 'N']

if args.is_ebt == 1:
    tp_list = get_pools_for_shortcode(args.shc, args.normal_or_staged, 'EBT')
else:
    tp_list = get_pools_for_shortcode(args.shc, args.normal_or_staged, 'Regular')

if args.timeperiod != '':
    tp_list = [tp for tp in tp_list if is_pool_in_tp(tp, args.timeperiod)]

print("\n".join(tp_list))
