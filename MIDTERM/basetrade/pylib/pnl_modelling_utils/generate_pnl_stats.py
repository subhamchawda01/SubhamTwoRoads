"""
Utility to generate data and get stdev of indicators and covariance matrix of indicators, invoked in pnl_modelling
"""

import os
import sys
import numpy as np
import subprocess
import datetime
import json
from math import sqrt
sys.path.append(os.path.expanduser('~/basetrade/'))
from grid.client.api import GridClient
from walkforward.definitions import execs
from scripts.ind_pnl_based_stats import fetch_pnl_stats_for_ilist_dates
GRID_URL = "http://10.1.4.15:5000"


def generate_data(shortcode, ilist, start_time, end_time, work_dir, run_datagen_dates):
    """
    Generates the pnl based stats by calling get_ind_stats_for_ilist.pl 
    :param shortcode: 
    :param given_date: end date for which the data is to be generated
    :param num_days: number of days for which the data is to be generated
    :param ilist: 
    :param start_time: 
    :param end_time: 
    :param work_dir: 
    :param skip_days_file: 
    :return: 
    """
    logfilename = os.path.join(work_dir, "main_log_file.txt")
    logfile = open(logfilename, 'w')

    print("WORK_DIR: " + work_dir)
    sys.stdout.flush()

    temp_ilist = os.path.join(work_dir, os.path.basename(ilist))
    os.system("cp " + ilist + " " + temp_ilist)

    run_datagen_dates_file = os.path.join(work_dir, "run_datagen_dates")
    w = open(run_datagen_dates_file, 'w')
    for dt in run_datagen_dates:
        w.write("%s\n" % dt)
    w.close()

    logfile.write("\nDatagen Start Time : " + str(datetime.datetime.now()) + "\n")
    logfile.flush()

    os.makedirs(os.path.join(work_dir, "IndicatorStats"))
    pnl_stats_file = os.path.join(work_dir, "IndicatorStats", "pnl_stats")
    output = fetch_pnl_stats_for_ilist_dates(shortcode, start_time, end_time, temp_ilist,
                                             dates=list(map(int, run_datagen_dates)))
    if output is not None:
        f = open(pnl_stats_file, 'w')
        f.write(output)
        f.close()
        logfile.write("\nDatagen End Time : " + str(datetime.datetime.now()) + "\n")

        logfile.flush()
        return logfilename

    datagen_json = {"ilist": temp_ilist, "job": "generate_data", "msecs": "10000", "l1events": "0", "trades": "0",
                    "start_time": str(start_time), "end_time": str(end_time),
                    "dates": list(map(str, run_datagen_dates)), "eco_mode": "0", "stats_args": "PNL_BASED_STATS"}
    grid_client = GridClient(server_url=GRID_URL, username=os.getenv("GRID_USERNAME"),
                             password=os.getenv("GRID_PASSWORD"), grid_artifacts_dir=work_dir)
    output_directory = grid_client.submit_job(json.dumps(datagen_json))
    datagen_artifacts_directory = os.path.join(output_directory, "artifacts", "datagen")
    sum_instances = 0

    indicators, _ = get_indicators(temp_ilist)
    num_indicators = len(indicators)
    mean_vec = np.zeros(num_indicators)
    sq_mean_vec = np.zeros(num_indicators)
    prod_mean_mat = np.zeros((num_indicators, num_indicators))
    for date in run_datagen_dates:
        date_file = os.path.join(datagen_artifacts_directory, str(date) + ".txt")
        if os.path.exists(date_file):
            f = open(date_file)
            ind_num = 0
            for line in f:
                tokens = line.split()
                if len(tokens) < 2:
                    continue
                if tokens[0] == "PNL_BASED_STATS":
                    mean_vec[ind_num] = mean_vec[ind_num] * (sum_instances / (sum_instances + float(tokens[1]))) + \
                        float(tokens[2]) * (float(tokens[1]) / (sum_instances + float(tokens[1])))
                    sq_mean_vec[ind_num] = sq_mean_vec[ind_num] * (sum_instances / (sum_instances + float(tokens[1]))) + \
                        float(tokens[3]) * (float(tokens[1]) / (sum_instances + float(tokens[1])))
                    for col in range(num_indicators):
                        c = prod_mean_mat[ind_num][col]
                        prod_mean_mat[ind_num][col] = c * (sum_instances / (sum_instances + float(tokens[1]))) + \
                            float(tokens[4 + col]) * (float(tokens[1]) / (sum_instances + float(tokens[1])))
                    if ind_num == num_indicators - 1:
                        sum_instances += float(tokens[1])
                    ind_num += 1

    stdev_vec = np.zeros(num_indicators)
    cov_matrix = np.zeros((num_indicators, num_indicators))

    for i in range(num_indicators):
        if sq_mean_vec[i] - mean_vec[i] * mean_vec[i] < 0:
            stdev_vec[i] = 0.0
        else:
            stdev_vec[i] = sqrt(sq_mean_vec[i] - mean_vec[i] * mean_vec[i])
        for j in range(num_indicators):
            cov_matrix[i][j] = prod_mean_mat[i][j] - mean_vec[i] * mean_vec[j]

    f = open(pnl_stats_file, 'w')
    f.write("STDEV ")
    f.write(" ".join(list(map(str, stdev_vec))))
    f.write("\n")
    f.write("COVARIANCE MATRIX\n")
    for i in range(num_indicators):
        f.write(" ".join(list(map(str, cov_matrix[i, :]))))
        f.write("\n")
    f.close()
    logfile.write("\nDatagen End Time : " + str(datetime.datetime.now()) + "\n")

    logfile.write("STDEV ")
    logfile.write(" ".join(list(map(str, stdev_vec))))
    logfile.write("\n")
    logfile.write("COVARIANCE MATRIX\n")
    for i in range(num_indicators):
        logfile.write(" ".join(list(map(str, cov_matrix[i, :]))))
        logfile.write("\n")
    logfile.flush()

    logfile.close()

    return logfilename


def get_indicators(ilist):

    indicators = []
    weights = []
    f = open(ilist, 'r')
    for line in f:
        tokens = line.split()
        if len(tokens) > 0 and tokens[0] == "INDICATOR":
            end_indx = len(tokens)
            for j in range(len(tokens)):
                if tokens[j][0] == '#':
                    end_indx = j
                    break
            indicator = ' '.join(x for x in tokens[2:end_indx])
            weights.append(float(tokens[1]))
            indicators.append(indicator)

    f.close()
    return indicators, weights


def get_covariance_matrix(pnl_stats_file, logfilename):
    """
    Given the pnl stats file, which is the ouput of get_ind_stats_for_ilist.pl, returns the standard deviations and
    covariance matrix of the data. Needs to be called after generate_data
    :param pnl_stats_file: path to pnl_stats file , which is output of get_ind_stats 
    :param logfilename: 
    :return: 
    """
    f = open(pnl_stats_file, 'r')
    standard_deviations = []
    read_covariance = False
    num_indicators = 0
    covariance_matrix = []
    for line in f:
        tokens = line.strip().split()
        if len(tokens) > 0 and tokens[0] == "STDEV":
            for i in range(1, len(tokens)):
                standard_deviations.append(float(tokens[i]))
            num_indicators = len(tokens) - 1
        elif len(tokens) > 0 and tokens[0] == "COVARIANCE" and tokens[1] == "MATRIX":
            read_covariance = True
        elif read_covariance:
            cov_row = []
            for i in range(0, num_indicators):
                cov_row.append(float(tokens[i]))
            covariance_matrix.append(cov_row)
            if len(covariance_matrix) == num_indicators:
                read_covariance = False
    f.close()

    standard_deviations = np.array(standard_deviations)
    covariance_matrix = np.array(covariance_matrix)

    logfile = open(logfilename, 'ab')
    np.savetxt(logfile, standard_deviations)
    np.savetxt(logfile, covariance_matrix)
    logfile.flush()
    logfile.close()

    return standard_deviations, covariance_matrix
