import os
import sys
from math import sqrt
import numpy as np
from copy import deepcopy
sys.path.append(os.path.expanduser("~/basetrade/"))

from scripts.ind_pnl_based_stats import fetch_pnl_stats_for_ilist_dates, read_master_ilist
from pylib.indstats_db_access_manager import *

def parse_pnl_stats_str(pnl_stats_str):
    """
    Given the pnl stats file, which is the ouput of get_ind_stats_for_ilist.pl, returns the standard deviations and
    covariance matrix of the data. Needs to be called after generate_data
    :param pnl_stats_file: path to pnl_stats file , which is output of get_ind_stats
    :return:
    """
    standard_deviations = []
    num_indicators = 0
    read_covariance = False
    covariance_matrix = []
    for line in pnl_stats_str.splitlines():
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

    standard_deviations = np.array(standard_deviations)
    covariance_matrix = np.array(covariance_matrix)
    return standard_deviations, covariance_matrix


def update_prod_ind_corr(last_sel_ind_idx, prod_ind_corr, ind_cov):
    len_inds = ind_cov.shape[0]
    for i in range(len_inds):
        if i == last_sel_ind_idx:
            continue
        nume = (prod_ind_corr[i]*sqrt(abs(ind_cov[i,i])) - ind_cov[i, last_sel_ind_idx]*prod_ind_corr[last_sel_ind_idx]/sqrt(abs(ind_cov[last_sel_ind_idx, last_sel_ind_idx])))
        denomi = sqrt(abs(ind_cov[i, i] - 1.0 * ind_cov[i, last_sel_ind_idx]**2 / ind_cov[last_sel_ind_idx, last_sel_ind_idx]))
        if abs(denomi) < 0.000001:
            prod_ind_corr[i] = 0.0
            continue
        prod_ind_corr[i] = nume/denomi
    ## Delete it if does not make sense to explicitly perform it
    prod_ind_corr[last_sel_ind_idx] = 0


def update_ind_cov(last_sel_ind_idx, ind_cov):
    len_inds = ind_cov.shape[0]
    for i in range(len_inds):
        for j in range(len_inds):
            if i==last_sel_ind_idx or j == last_sel_ind_idx:
                    continue
            # ind_cov[i,j] = ind_cov[i,j] - 1.0*ind_cov[i, last_sel_ind_idx]*ind_cov[j, last_sel_ind_idx]/ind_stdev[last_sel_ind_idx]**2
            ind_cov[i, j] = ind_cov[i, j] - 1.0 * (ind_cov[i, last_sel_ind_idx] * ind_cov[j, last_sel_ind_idx] / ind_cov[last_sel_ind_idx, last_sel_ind_idx])
    for i in range(len_inds):
        ind_cov[i, last_sel_ind_idx] = 0
        ind_cov[last_sel_ind_idx, i] = 0


def update_sel_ind_idx(last_sel_ind_idx, ind_cov, prod_ind_corr, min_candidate_corr, eligible):
    ## Both the following functions require equivalent copy of ind_stdev and ind_cov.. calling
    ## one before other causes different copies.
    if last_sel_ind_idx is not None:
        # This update should mandatarily happen before any ind_cov update as later don't depend on this but opposite is not true
        update_prod_ind_corr(last_sel_ind_idx, prod_ind_corr, ind_cov)
        # update_ind_stdev(last_sel_ind_idx, ind_stdev, ind_cov)
        update_ind_cov(last_sel_ind_idx, ind_cov)
    len_inds = ind_cov.shape[0]
    new_sel_ind_idx = None
    max_yet = 0
    for i in range(len_inds):
        if eligible[i]:
            if prod_ind_corr[i] > max_yet and prod_ind_corr[i] > min_candidate_corr:
                new_sel_ind_idx = i
                max_yet = prod_ind_corr[i]
    return new_sel_ind_idx

def stats_based_ind_sel(shortcode, ilist, dates, start_time, end_time, regdata_args, regdata_process_filter, reg_string, preprocessed_ilist):
    tokens = reg_string.split()[1:]
    if len(tokens) < 4:
        raise ValueError("min tokens 4 required for fast_fsrr method, " + regress_exec_params)
    min_initial_corr, max_mutual_corr, min_candidate_corr, max_model_size = list(map(float, tokens))

    ind_cov = []
    prod_ind_corr = []
    #print("initial ilist: ", ilist, "\n")

    ## Assumption: Number of indicators in ind_cov and prod_ind_corr is same. In case any one of them is larger set,
    ## then we would have to take intersection of the two sets.
    #print("\n######fetching pnl stats")
    pnl_stats_str = fetch_pnl_stats_for_ilist_dates(shortcode, start_time, end_time, ilist, dates=dates)
    if pnl_stats_str is None:
        #print("No pnl stats for at least 70% of dates. Exiting\n")
        return
    _, ind_cov = parse_pnl_stats_str(pnl_stats_str)

    regdata_args = regdata_args.split()
    predalgo = regdata_args[0]
    pred_duration = regdata_args[1]
    filters = regdata_process_filter

    IndStatsObject = IndStatsDBAcessManager()
    IndStatsObject.open_conn()
    #print("\n#####fetching indicator stats")
    prod_ind_corr = IndStatsObject.get_ind_stats_for_ilist_dates(shortcode, start_time, end_time, predalgo, pred_duration, filters, ilist, dates)
    if prod_ind_corr is None:
        #print("No prod ind correlation for at least 70% of dates. Exiting\n")
        return
    prod_ind_corr_sign = list(map(lambda x: int(x)*2-1 , prod_ind_corr>=0))

    #ind_cov = np.random.randn(100, 100)
    #prod_ind_corr = np.random.randn(100)
    #print(ind_cov.shape)
    #print(prod_ind_corr.shape)

    len_inds = ind_cov.shape[0]

    eligible = list(map(lambda x: int(abs(x) >= min_initial_corr), prod_ind_corr))
    for i in range(len_inds):
        if not eligible[i]:
            continue
        for j in range(i+1, len_inds):
            if abs(ind_cov[i][j])/(sqrt(ind_cov[i][i])*sqrt(ind_cov[j][j])) > max_mutual_corr:
                eligible[j] = 0

    sel_ind_idx = None
    sel_inds = []
    while len(sel_inds) < max_model_size:
        sel_ind_idx = update_sel_ind_idx(sel_ind_idx, ind_cov, prod_ind_corr, min_candidate_corr, eligible)
        if sel_ind_idx is None:
            break
            # sys.exit()
        else:
            eligible[sel_ind_idx] = 0
            sel_inds.append(sel_ind_idx)
            ## If we should update statistics related to this indicator here
    indicators, _ = read_master_ilist(ilist)
    with open(preprocessed_ilist, "w") as fout:
        for ind_idx in sel_inds:
            fout.write("INDICATOR {:.2f} {}\n".format(prod_ind_corr_sign[ind_idx], indicators[ind_idx]))
        fout.close()
    #print("from fast_fsrr, ilist: ", preprocessed_ilist)


if __name__ == "__main__" :
    shortcode = "6J_0"
    ilist = "/home/araj/temp_ilists/ind_sel_test"
    dates = ["20171210", "20171211", "20171212", "20171213", "20171214", "20171215", "20171216", "20171217", "20171218", "20171219"]
    start_time = "JST_905"
    end_time = "EST_800"
    regdata_args = "na_e3 100"
    regdata_process_filter = "fsg.5"
    reg_string = "FAST_FSRR 0.04 0.9 0.01 10"
    preprocessed_ilist = "/home/araj/preprocessed_ilist"
    stats_based_ind_sel(shortcode, ilist, dates, start_time, end_time, regdata_args, regdata_process_filter, reg_string, preprocessed_ilist)
