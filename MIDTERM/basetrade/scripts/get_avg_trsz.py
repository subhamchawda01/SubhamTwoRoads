#!/usr/bin/python

import sys
import os
import argparse
import subprocess

sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.utils.date_utils import calc_next_week_day
from walkforward.utils.date_utils import calc_prev_week_day
from walkforward.definitions import execs



def GetAvgTradeSize(shc,date,numDays):

    avgtradesSum = 0
    count = 0
    avg_sample_cmd = [execs.execs().avg_samples, shc, str(date), str(numDays), "BRT_905", "BRT_1540", "0", "RollingAvgTradeSize300"]
    # print(avg_sample_cmd)
    process = subprocess.Popen(' '.join(avg_sample_cmd), shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    # print(err)
    if err is not None:
        err = err.decode('utf-8')
    errcode = process.returncode
    if len(err) > 0:
        raise ValueError("Error in fetching sample data")
    avgtr = str(out.strip().split()[2])

    '''for day in range(0, int(numDays)):
        date = calc_prev_week_day(date, 1)
        out = os.popen("~/LiveExec/bin/mkt_trade_logger SIM " + shc + " " + str(date) + " | grep OnTrade | awk '{sum+=$5 ;count+=1}END{print sum/count}'").read()
        try:
            avgtradesSum += float(out)
            count += 1
        except ValueError:
            continue

    avgtr = float(avgtradesSum / count)'''
    avgtr = float(avgtr)
    return avgtr

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-shc', dest='shc', help="shortcode", type=str, required=True)
    parser.add_argument('-date', dest='date', help="date", type=str, required=True)
    parser.add_argument('-numDays',dest='numDays', help='numDays', type=str, required=True)

    args = parser.parse_args()

    print(GetAvgTradeSize(args.shc,args.date,args.numDays))
