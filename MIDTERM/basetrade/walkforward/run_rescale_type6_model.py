#!/usr/bin/env python

"""

Runs the rescale script

"""
import os
import sys
import argparse
import datetime

sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.wf_db_utils.rescale_type6_model import rescale_type6_model


parser = argparse.ArgumentParser()
parser.add_argument('-c', dest='configname', help="the walk-forward-config identifier", type=str, required=True)
parser.add_argument('-f', dest='rescale_constant', help='constat value to rescale', type=float, required=True)
parser.add_argument('-sd', dest='start_date', help='start date since we want to rescale model coeffs',
                    type=int, default=19700101, required=False)
parser.add_argument('-ed', dest='end_date', help='end date till we want to rescale model coeffs', type=int,
                    default=int(datetime.date.today().strftime("%Y%m%d")), required=False)

args = parser.parse_args()


rescale_type6_model(args.configname, args.rescale_constant, args.start_date, args.end_date)
