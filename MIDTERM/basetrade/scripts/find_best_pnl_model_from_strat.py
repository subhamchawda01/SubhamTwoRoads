#!/usr/bin/env python
"""

Script to get best weight in pnl space

"""
import os
import sys
import subprocess


sys.path.append(os.path.expanduser('~/basetrade/'))
sys.path.append(os.path.expanduser('~/grid/'))

from scripts.find_best_pnl_model import get_best_pnl_model
from walkforward.definitions import execs
from walkforward.utils.date_utils import calc_prev_week_day

if __name__ == "__main__":

    if len(sys.argv) <= 5:
        print("USAGE: <strat_filename> <end_date> <num_days> <choose_top_strats> <max_sum>")
        sys.exit(0)

    strat_name = sys.argv[1]
    date = sys.argv[2]
    num_days = sys.argv[3]
    choose_top_strats = sys.argv[4]
    max_sum = sys.argv[5]

    with open(strat_name, 'r') as strat_file:
        strat_line = strat_file.readline()
        strat_words = strat_line.split()
        shortcode = strat_words[1]
        execlogic = strat_words[2]
        ilist = strat_words[3]
        param_file = strat_words[4]
        start_time = strat_words[5]
        end_time = strat_words[6]

    start_date = calc_prev_week_day(int(date), int(num_days))
    model_stdev_cmd = [execs.execs().get_stdev_model, ilist, str(start_date), str(date), start_time, end_time]
    process = subprocess.Popen(' '.join(model_stdev_cmd), shell=True,
                               stderr=subprocess.PIPE,
                               stdout=subprocess.PIPE)
    out, err = process.communicate()
    errcode = process.returncode

    # decode the utf from byte
    err = err.decode('utf-8') if err else err
    out = out.decode('utf-8') if out else out

    target_stdev = out.split(' ')[0]

    print("Target Stdev: " + str(target_stdev))

    get_best_pnl_model(shortcode, execlogic, ilist, [param_file],
                       date, num_days, start_time, end_time,
                       [target_stdev], choose_top_strats, max_sum)
