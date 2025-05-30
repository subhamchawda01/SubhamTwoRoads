#!/usr/bin/env python
from __future__ import print_function
import pdb
import os
import sys
import time
import getpass
import argparse
import subprocess
import math

sys.path.append(os.path.expanduser('~/basetrade/'))
sys.path.append(os.path.expanduser('~/grid/'))
from walkforward.utils.date_utils import calc_prev_week_day
from walkforward.definitions import execs


# print(os.system("/home/dvctrader/basetrade/scripts/get_avg_samples.pl "+ shortcode + " "+

def get_pred_from_feature(shortcode, end_date, num_days_to_compute_feature, start_time, end_time, feature, feature_value):
    # print("GetPredDur",shortcode,end_date,num_days_to_compute_feature,start_time,end_time,feature,feature_value)
    avg_feature_val_cmd = [execs.execs().avg_samples, shortcode, str(end_date), str(
        num_days_to_compute_feature), start_time, end_time, '0', feature]
    avg_feature_out = subprocess.Popen(' '.join(avg_feature_val_cmd), shell=True, stdout=subprocess.PIPE)
    Historical_featVal = float(avg_feature_out.communicate()[0].decode('utf-8').strip().split()[-1])
    # print("feature_value",feature_value)
    #print("Historical Value",Historical_featVal)
    if float(Historical_featVal) == 0:
        pred_Dur = 300
    else:
        if feature == "VOL":
            pred_Dur = 300 * float(feature_value) / float(Historical_featVal)
        elif feature == "STDEV":
            pred_Dur = 300 * ((float(feature_value) / float(Historical_featVal))**2)
        else:
            pred_Dur = 300
    #print("Pred Duration",pred_Dur)

    return pred_Dur


if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument(dest='shortcode', help="Shortcode of Product", type=str)
    parser.add_argument(dest='end_date', help='End Date', type=int)
    parser.add_argument(dest='num_days_to_compute_feature',
                        help='Num days for which feature value will be computed', default=20, type=int)
    parser.add_argument(dest='start_time', help='Start Time', type=str)
    parser.add_argument(dest='end_time', help='End Time', type=str)
    parser.add_argument(dest='feature', help='Feature like STDEV/VOL', default='STDEV', type=str)
    parser.add_argument(dest='feature_value', help='Feature Value', type=str)
    args = parser.parse_args()
    out = get_pred_from_feature(args.shortcode, args.end_date, args.num_days_to_compute_feature,
                                args.start_time, args.end_time, args.feature, args.feature_value)
    print(out, end="")
