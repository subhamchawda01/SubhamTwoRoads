'''
Utility script involved with usage of conditional variable in pnl_modelling
'''
import math
import os
import numpy as np
import subprocess
import sys

sys.path.append(os.path.expanduser('~/basetrade/'))
from walkforward.utils.prepare_tdata import run_datagen
from walkforward.definitions import execs
from pylib.pnl_modelling_utils.ilist_utils import read_indicators_from_ilist


def learn_sigmoid_params(cv, cv_percentile_list, work_dir):
    """
    Given a conditional variable, and the percentiles at which we want the sigmoid to take 0 and 1 probability, it
    computes the a and b parameters for sigmoid function. 
    :param cv: the index of conditional variable in the ilist
    :param cv_percentile_list: list of tuples (lower_percentile,upper_percentile) for which a and b need to be calculated
    :param work_dir: the work directory containing the file cv_data_file, with the datagen output
    :return: return the list of (a,b) tuples corresponding to every combination
    """
    cv_data_file = os.path.join(work_dir, "cv_data_file")
    cv_data = np.loadtxt(cv_data_file)
    params = []
    k1 = math.log(1.0 / 0.95 - 1)
    k2 = math.log(1.0 / 0.05 - 1)
    for (l, u) in cv_percentile_list:
        l = l * 100.0
        u = u * 100.0
        lower_percentile = np.percentile(cv_data[:, 4 + cv], l, axis=0)
        upper_percentile = np.percentile(cv_data[:, 4 + cv], u, axis=0)
        b = -1.0 * (upper_percentile * k2 - lower_percentile * k1) / (upper_percentile - lower_percentile)
        a = -1.0 * (b + k1) / (upper_percentile)
        params.append((a, b))
    return params


def generate_cv_data(cv_ilist, start_time, end_time, end_date, num_days, shortcode, work_dir):
    """
    The function runs datagen on the ilist containing the conditional variables , and stores the catted output in
    work_dir/cv_datafile
    :param cv_ilist: ilist containing the conditional variables
    :param start_time: 
    :param end_time: 
    :param end_date: 
    :param num_days: 
    :param shortcode: 
    :param work_dir: 
    """
    dates_cmd = [execs.execs().get_dates, shortcode, str(num_days), str(end_date)]
    process = subprocess.Popen(' '.join(dates_cmd), shell=True,
                               stderr=subprocess.PIPE,
                               stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')
    errcode = process.returncode
    if len(err) > 0:
        raise ValueError("Error in retrieving dates")
    dates = out.split()
    catted_cv_file = os.path.join(work_dir, "cv_data_file")
    for date in dates:
        tdata_file = os.path.join(work_dir, "t_dgen_tempfile")
        out, err, errcode = run_datagen(cv_ilist, date, start_time, end_time, tdata_file, "1000", "0", "0")
        os.system("cat " + tdata_file + " >> " + catted_cv_file)


def create_cv_ilist(cv_list, cv_indicator_percentile_combinations, ilist, new_ilist, work_dir,
                    shortcode, start_time, end_time, end_date, num_days):
    """
    Generates the ilist and given some conditional variables and the associated indicators and the conditional variable
    combinations of percetntile, it generates a new ilist with the conditional variables associated with every indicator 
    for all possible combinations of percentile. The new ilist will have sigmoid(CV)*Indicator, for every association and 
    percentile combination
    :param cv_list: list of conditional variables
    :param cv_indicator_percentile_combinations: list of associated combinations of indicators and conditional variables
     in ilist [(conditional_variable, indicator, (lower_percentile, upper_percentile))]
    :param ilist: ilist to run pnl modelling on
    :param new_ilist: new ilist formed after combinining conditonal variables
    :param work_dir: 
    :param cv_percentile_list: dictionary of cv_index: [(lower percentile, upper percentile)] 
    :param shortcode: 
    :param start_time: 
    :param end_time: 
    :param end_date: 
    :param num_days: 
    """
    f = open(ilist, 'r')
    pre_indicator_lines = []
    for line in f:
        tokens = line.strip().split()
        if len(tokens) > 0 and tokens[0] == "INDICATOR":
            break
        pre_indicator_lines.append(line.strip())
    f.close()
    cv_ilist = os.path.join(work_dir, "cv_ilist")
    w = open(cv_ilist, 'w')

    for line in pre_indicator_lines:
        w.write(line + "\n")
    for ind in cv_list:
        w.write("INDICATOR 1.00 " + ind + "\n")
    w.close()

    generate_cv_data(cv_ilist, start_time, end_time, end_date, num_days, shortcode, work_dir)
    indicators, weights = read_indicators_from_ilist(ilist)
    ind_cv = {}

    for ind in indicators:
        ind_cv[ind] = []

    for combination in cv_indicator_percentile_combinations:
        cv = combination[0]
        ind = combination[1]
        percentile = combination[2]
        cv_params_list = learn_sigmoid_params(cv - 1, [percentile], work_dir)
        num_tokens_in_cv = cv_list[cv - 1].split()
        for (a, b) in cv_params_list:
            ind_cv[indicators[ind - 1]].append("Expression SIGMOID " + str(len(num_tokens_in_cv) - 1) + " 1.00 " + cv_list[cv - 1] + " PARAMS 2 " + str(
                a) + " " + str(b))

    new_indicators = []
    for indicator, weight in zip(indicators, weights):
        ind_tokens = indicator.split()
        if weight > 0:
            sign = "1.00"
        else:
            sign = "-1.00"
        indicator_string = "INDICATOR " + sign + " " + indicator
        new_indicators.append(indicator_string)
        for cv_string in ind_cv[indicator]:
            print(cv_string)
            indicator_string = "INDICATOR " + sign + " Expression MULT " + str(
                len(cv_string.split()) - 1) \
                + " 1.00 " + cv_string + " " + str(
                len(indicator.split()) - 1) + " 1.00 " + indicator
            new_indicators.append(indicator_string)

    w = open(new_ilist, 'w')
    for line in pre_indicator_lines:
        w.write(line + "\n")
    for ind in new_indicators:
        w.write(ind + "\n")
    w.write("INDICATOREND")
    w.close()
