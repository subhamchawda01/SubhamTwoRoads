#!/usr/bin/env python

import numpy as np
import sys
import subprocess
import os.path
import pandas as pd
from scipy.stats import itemfreq

np.set_printoptions(suppress=True)

X = []
Y = []
date = []

train_idx_ = []
test_idx_ = []
algo_to_performance_ = {}
workdir = ""

test_reg_data_file_ = ""
train_reg_data_file_ = ""
train_date_file_ = ""
test_date_file_ = ""

#@profile


def ReadFileAndPerformTestTrainPartition(filename):
    global train_idx_
    global test_idx_
    global X
    global Y
    global date
    # faster

    t_data_ = pd.read_csv(filename, sep=' ').values
    numcols_ = t_data_.shape[1]  # observe that the last colun contains the date information
    #Y = t_data_[:,0]
    #X = t_data_[:,1:numcols_]
    indices_ = np.argsort(t_data_[:, 0])
    Y = np.array([t_data_[i][0] for i in indices_])
    X = np.array([t_data_[i][1:numcols_ - 1] for i in indices_])
    date = np.array([t_data_[i][numcols_ - 1] for i in indices_])

    numrows_ = Y.shape[0]
    slice_ = int(0.1 * numrows_)
    for current_start_index_ in xrange(0, numrows_, slice_):
        current_end_index_ = min(current_start_index_ + slice_, numrows_)
        range_ = current_end_index_ - current_start_index_
        shuffled_idx_ = current_start_index_ + np.random.permutation(range_)
        boundary_ = int(2.0 / 3 * range_)  # keep 1/3rd data for test
        for i in xrange(boundary_):  # this seems more memory efficient
            train_idx_.append(shuffled_idx_[i])
        for i in xrange(boundary_, range_):
            test_idx_.append(shuffled_idx_[i])

    train_idx_ = np.array(train_idx_).astype(int)
    test_idx_ = np.array(test_idx_).astype(int)


#@profile
def GenerateRegDataFile(train):
    global X
    global Y
    global date
    if train == True:
        train_reg_data_ = np.column_stack((Y[train_idx_], X[train_idx_, :])).tolist()
        train_date_ = date[train_idx_].astype(int).tolist()
        consolidated_ = sorted(zip(train_date_, train_reg_data_))
        train_reg_data_ = np.array([x for (y, x) in consolidated_])
        train_date_ = np.array([y for (y, x) in consolidated_])
        np.savetxt(train_reg_data_file_, train_reg_data_, fmt="%.10f")
        np.savetxt(train_date_file_, np.fliplr(itemfreq(train_date_)), fmt="%d")
    else:
        test_reg_data_ = np.column_stack((Y[test_idx_], X[test_idx_, :])).tolist()
        np.savetxt(test_reg_data_file_, test_reg_data_, fmt="%.10f")


#@profile
def _main_():
    global workdir
    global test_reg_data_file_
    global train_reg_data_file_
    global train_date_file_
    global test_date_file_
    if len(sys.argv) < 5:
        print "Usage:<script><workdir><regdata><test performance><number of folds><regression parameters>"
        exit(1)
    if len(sys.argv) == 5:
        print "Atleast one regression algorithm must be specified"
        exit(1)
    workdir = sys.argv[1]
    train_reg_data_file_ = os.path.join(workdir, "tmp_train_reg_data")
    test_reg_data_file_ = os.path.join(workdir, "tmp_test_reg_data")
    train_date_file_ = os.path.join(workdir, "tmp_train_datefile")
    test_date_file_ = os.path.join(workdir, "tmp_test_datefile")
    MODELSCRIPTS_DIR = "~/basetrade_install/ModelScripts/"
    try:
        ReadFileAndPerformTestTrainPartition(sys.argv[2])
    except:  # bad input file name,should not arise normally
        print "FSLR,0.01,0,0,0.7,18 2.0"  # revert to the default regression algorithm
        exit(0)
    GenerateRegDataFile(train=True)
    processes = []
    params = []
    for i in xrange(5, len(sys.argv)):  # need to bypass for those algo which have cross validation inherent in their setup
        t_param_ = sys.argv[i].replace(",", " ")
        params.append(t_param_)
        t_reg_type_ = t_param_.strip().split()[0]
        if t_reg_type_ == "BOOSTING":
            processes.append(subprocess.Popen(MODELSCRIPTS_DIR + "/adaboost.py BOOSTING" + " " + train_reg_data_file_ + " " +
                                              t_param_.replace(t_reg_type_, "") + " 2>/dev/null | tail -n1", shell=True, stdout=subprocess.PIPE, bufsize=1))
        elif t_reg_type_ == "TREEBOOSTING":
            processes.append(subprocess.Popen(MODELSCRIPTS_DIR + "/adaboost.py TREEBOOSTING " + train_reg_data_file_ + " " +
                                              t_param_.replace(t_reg_type_, "") + " 2>/dev/null | tail -n1", shell=True, stdout=subprocess.PIPE, bufsize=1))
        elif t_reg_type_ == "CONSOLIDATEDBOOSTING":
            processes.append(subprocess.Popen(MODELSCRIPTS_DIR + "/consolidated_adaboost.py CONSOLIDATEDBOOSTING" + " " + train_reg_data_file_ + " " +
                                              train_date_file_ + " " + t_param_.replace(t_reg_type_, "") + " 2>/dev/null | tail -n1", shell=True, stdout=subprocess.PIPE, bufsize=1))
        elif t_reg_type_ == "CONSOLIDATEDTREEBOOSTING":
            processes.append(subprocess.Popen(MODELSCRIPTS_DIR + "/consolidated_adaboost.py CONSOLIDATEDTREEBOOSTING" + " " + train_reg_data_file_ + " " +
                                              train_date_file_ + " " + t_param_.replace(t_reg_type_, "") + " 2>/dev/null | tail -n1", shell=True, stdout=subprocess.PIPE, bufsize=1))
        elif t_reg_type_ == "COARSEBOOSTING":
            processes.append(subprocess.Popen(MODELSCRIPTS_DIR + "/coarse_adaboost.py COARSEBOOSTING" + " " + train_reg_data_file_ + " " +
                                              train_date_file_ + " " + t_param_.replace(t_reg_type_, "") + " 2>/dev/null | tail -n1", shell=True, stdout=subprocess.PIPE, bufsize=1))
        elif t_reg_type_ == "COARSETREEBOOSTING":
            processes.append(subprocess.Popen(MODELSCRIPTS_DIR + "/coarse_adaboost.py COARSETREEBOOSTING" + " " + train_reg_data_file_ + " " +
                                              train_date_file_ + " " + t_param_.replace(t_reg_type_, "") + " 2>/dev/null | tail -n1", shell=True, stdout=subprocess.PIPE, bufsize=1))
        elif t_reg_type_ == "RANDOMFOREST":
            processes.append(subprocess.Popen(MODELSCRIPTS_DIR + "/randomForest_model.R " + train_reg_data_file_ + " " + workdir + "/tmp_regoutput " +
                                              t_param_.replace(t_reg_type_, "") + ";cat " + workdir + "/tmp_regoutput | tail -n1", shell=True, stdout=subprocess.PIPE, bufsize=1))
        elif t_reg_type_ == "SIGLR":
            processes.append(subprocess.Popen(MODELSCRIPTS_DIR + "/SIGLR_grad_descent_2.R " + train_reg_data_file_ + " " + workdir + "/tmp_regoutput " +
                                              t_param_.replace(t_reg_type_, "") + ";tac " + workdir + "/tmp_regoutput | sed \'7q;d\'", shell=True, stdout=subprocess.PIPE, bufsize=1))
        elif t_reg_type_ == "SIGLRF":
            processes.append(subprocess.Popen(MODELSCRIPTS_DIR + "/SIGLR_grad_descent.R " + train_reg_data_file_ + " " + workdir + "/tmp_regoutput " +
                                              t_param_.replace(t_reg_type_, "") + ";tac " + workdir + "/tmp_regoutput | sed \'6q;d\'", shell=True, stdout=subprocess.PIPE, bufsize=1))
        else:
            processes.append(subprocess.Popen(MODELSCRIPTS_DIR + "/cross_validate_model.py " + workdir + " " + train_reg_data_file_ +
                                              " " + sys.argv[4] + " 2 " + t_param_ + " 2>/dev/null | sed -n \'x;$p\'", shell=True, stdout=subprocess.PIPE, bufsize=1))
    outputs = [process.communicate()[0] for process in processes]
    for i in xrange(len(params)):
        try:
            performance = float(outputs[i].strip().split()[1])
        except:
            performance = -100  # since the user messed up the input we are unable to provide a valid output
        algo_to_performance_[params[i]] = performance
    best_model_, best_performance_ = max(algo_to_performance_.items(), key=lambda x: x[1])
    GenerateRegDataFile(train=False)
    print best_model_, (1 + float(sys.argv[3])) * (1 - best_performance_)


_main_()
