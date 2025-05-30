#!/usr/bin/env python

import numpy as np
import pandas as pd
import sys
import subprocess
import os.path

X = []
Y = []
folds = 5
partitions = []
errors_ = []
trained_models_ = []
regressalgo = ""
execpath_ = ""
workdir_ = ""
additional_param_ = {}
max_allowed_error_ = 1
all_range_ = []
np.set_printoptions(suppress=True)

#@profile


def ReadFile(filename):
    global X
    global Y
    global all_range_
    global max_allowed_error_
    t_data_ = pd.read_csv(filename, sep=' ').values
    Y = t_data_[:, 0]
    X = t_data_[:, 1:t_data_.shape[1]]
    indices_ = np.argsort(Y)
    Y = np.array([Y[i] for i in indices_])
    X = np.array([X[i] for i in indices_])
    all_range_ = np.arange(X.shape[0])
    if max_allowed_error_ == 0:  # choose the error from the data
        #		max_allowed_error_ = np.median(np.abs(Y - np.median(Y)))			#perhaps MAD or any such metric is more desirable?
        max_allowed_error_ = 1.5 * np.std(np.abs(Y - np.median(Y)))
#@profile


def PerformPartition():
    global partitions

    partitions = [[] for i in xrange(folds)]
    numcols_ = Y.shape[0]
    slice_ = int(0.1 * numcols_)

    for current_start_index_ in xrange(0, numcols_, slice_):
        current_end_index_ = min(current_start_index_ + slice_, numcols_)
        shuffled_idx_ = current_start_index_ + np.random.permutation(current_end_index_ - current_start_index_)
        distribution_length_ = int(shuffled_idx_.shape[0] / folds)
        begin_ = i * distribution_length_
        partitions = [np.hstack((partitions[i], shuffled_idx_[begin_:min(
            begin_ + distribution_length_, shuffled_idx_.shape[0])])) for i in xrange(0, folds)]

    partitions = np.array(partitions).astype(int)

#@profile


def TrainModel(training_idx_):
    # use subprocess to obtain the output
    # create the temp regdata file
    global additional_param_
    global regressalgo
    global execpath_
    line = 0

    np.savetxt(os.path.join(workdir_, "tmp_reg_data"), np.column_stack(
        (Y[training_idx_], X[training_idx_, :])), fmt="%.12f")

    reg_file_ = os.path.join(workdir_, "tmp_reg_data")
    output_file_ = os.path.join(workdir_, "tmp_output")

    if regressalgo == "L1_REGRESSION":
        status = subprocess.call(execpath_ + " -i " + reg_file_ + " -o " + output_file_ + " -c " +
                                 additional_param_["cutoff_l1_reg"] + " -r " + additional_param_["change_ratio"], shell=True, bufsize=1)

    elif regressalgo == "PCA_REGRESSION":
        status = subprocess.call(execpath_ + " " + reg_file_ + " " + additional_param_["num_pca_comp"] + " " + additional_param_[
                                 "max_model_size"] + " " + additional_param_["pca_heuristic"] + " " + additional_param_["min_correlation"] + " " + output_file_, shell=True, bufsize=1)

    elif regressalgo == "LM":
        status = subprocess.call(execpath_ + " " + reg_file_ + " " + output_file_ +
                                 " 2>&1 1>/dev/null", shell=True, bufsize=1)

    elif regressalgo in ["LASSO", "SLASSO", "LR"]:
        status = subprocess.call(execpath_ + " " + reg_file_ + " " +
                                 additional_param_["max_model_size"] + " " + output_file_ + " 2>&1 1>/dev/null", shell=True, bufsize=1)
    elif regressalgo in ["RIDGE"]:
        status = subprocess.call(execpath_ + " " + reg_file_ + " " + additional_param_["df"] + " " + additional_param_[
                                 "min_variance_explained"] + " " + output_file_ + " 2>&1 1>/dev/null", shell=True, bufsize=1)
    elif regressalgo in ["EARTH", "FSRR", "FSRLM", "FSVLR"]:
        status = subprocess.call(execpath_ + " " + reg_file_ + " " + additional_param_["regulariser"] + " " + additional_param_["min_correlation"] + " " + additional_param_["first_indep_weight"] + " " + additional_param_[
                                 "must_include_first_k_independents"] + " " + additional_param_["max_indep_correlation"] + " " + output_file_ + " " + additional_param_["max_model_size"] + " IVALIDFILE 2>&1 1>/dev/null", shell=True, bufsize=1)

    elif regressalgo in ["FSRMFSS", "FSRMSH", "FSRSHRSQ"]:
        status = subprocess.call(execpath_ + " " + reg_file_ + " " + additional_param_["regulariser"] + " " + additional_param_["min_correlation"] + " " + additional_param_["must_include_first_k_independents"] + " " + additional_param_[
                                 "first_indep_weight"] + " " + additional_param_["max_indep_correlation"] + " " + output_file_ + " " + additional_param_["max_model_size"] + " IVALIDFILE 2>&1 1>/dev/null", shell=True, bufsize=1)
    elif regressalgo == "MULTLR":
        status = subprocess.call(execpath_ + " " + reg_file_ + " " + additional_param_["regulariser"] + " " + additional_param_["min_correlation"] + " " + additional_param_[
                                 "must_include_first_k_independents"] + " " + additional_param_["max_indep_correlation"] + " " + output_file_ + " " + additional_param_["max_model_size"] + " IVALIDFILE 2>&1 1>/dev/null", shell=True, bufsize=1)
    else:
        status = subprocess.call(execpath_ + " " + reg_file_ + " " + additional_param_["min_correlation"] + " " + additional_param_["first_indep_weight"] + " " + additional_param_[
                                 "must_include_first_k_independents"] + " " + additional_param_["max_indep_correlation"] + " " + output_file_ + " " + additional_param_["max_model_size"] + " IVALIDFILE 2>&1 1>/dev/null", shell=True, bufsize=1)

    status = subprocess.call("rm " + workdir_ + "/tmp_reg_data", shell=True)

    # obtain weights from tmp_output
    weights_ = np.zeros(X.shape[1])
    if os.path.exists(output_file_):
        with open(output_file_) as file:
            for line in file:
                line = line.strip().split()
                if regressalgo in ["LM", "LASSO", "SLASSO", "LR", "RIDGE"]:
                    idx_ = int(line[0]) - 1
                    weights_[idx_] = float(line[1])
                else:
                    if line[0] == "OutCoeff":
                        idx_ = int(line[1])
                        weights_[idx_] = float(line[2])
        file.close()
        status = subprocess.call("rm " + output_file_, shell=True, bufsize=1)

    return weights_

#@profile


def MSE(trained_model_, test_idx_):
    Y_predict = np.dot(X[test_idx_, :], np.transpose(trained_model_))
    # adjusted R square
    return ((np.sum((Y_predict - Y[test_idx_])**2)) / (np.sum((Y[test_idx_] - np.mean(Y[test_idx_]))**2))) * (test_idx_.shape[0] - 1) * 1.0 / (test_idx_.shape[0] - trained_model_.shape[0])

#@profile


def CrossValidation():

    global X
    global Y
    global trained_models_
    global errors_

    for i in xrange(0, folds):
        training_idx_ = np.setdiff1d(all_range_, partitions[i])
        trained_models_.append(TrainModel(training_idx_))
        errors_.append(MSE(trained_models_[i], partitions[i]))
#@profile


def GetBestModel():
    bestmodelidx_ = errors_.index(min(errors_))
    return trained_models_[bestmodelidx_], bestmodelidx_

#@profile


def ExtractModel():
    global errors_
    global max_allowed_error_
    avg_error = np.mean(errors_)
    if avg_error < max_allowed_error_:
        best_model_, best_idx_ = GetBestModel()
    else:
        best_model_ = np.array([])
    # Do rest of the stuff here
    if best_model_.shape[0] == 0:
        print "No model satisified the error constraint"
        print "AdjustedRSquare -1"
        print "RSquared -1"
        print "Correlation -1"
    else:
        for i in xrange(best_model_.shape[0]):
            if best_model_[i] != 0:
                if regressalgo not in ["LM", "LASSO", "SLASSO", "LR", "RIDGE"]:
                    print "OutCoeff", i, best_model_[i]
                else:
                    print i + 1, best_model_[i]
        # print "AdjustedRsquare",1-errors_[best_idx_]
        # print "RSquared",1-errors_[best_idx_] / ((X[partitions[best_idx_]].shape[0] - 1)*1.0/(X[partitions[best_idx_]].shape[0] - best_model_.shape[0]))
        # print "Correlation",np.sqrt(1-errors_[best_idx_] / ((X[partitions[best_idx_]].shape[0] - 1)*1.0/(X[partitions[best_idx_]].shape[0] - best_model_.shape[0])))
        print "AdjustedRsquare", 1 - avg_error
        print "RSquared", 1 - avg_error / ((X[partitions[best_idx_]].shape[0] - 1) * 1.0 / (X[partitions[best_idx_]].shape[0] - best_model_.shape[0]))
        print "Correlation", np.sqrt(1 - avg_error / ((X[partitions[best_idx_]].shape[0] - 1) * 1.0 / (X[partitions[best_idx_]].shape[0] - best_model_.shape[0])))

#@profile


def _main_():

    global folds
    global max_allowed_error_
    global regressalgo
    global execpath_
    global additional_param_
    global workdir_

    MODELSCRIPTS_DIR = "~/basetrade_install/ModelScripts/"
    LIVE_BIN_DIR = "~/LiveExec/bin/"

    if len(sys.argv) < 5:
        print "Usage:<script><workdir><regdata><folds><max allowed error><regressalgo><algo params>"
        exit(1)

    workdir_ = sys.argv[1]
    filename_ = sys.argv[2]
    folds = int(sys.argv[3])
    max_allowed_error_ = float(sys.argv[4])
    regressalgo = sys.argv[5]

    additional_param_["regulariser"] = "0.01"
    additional_param_["min_correlation"] = "0.01"
    additional_param_["first_indep_weight"] = "0.0"
    additional_param_["must_include_first_k_independents"] = "-1"
    additional_param_["max_indep_correlation"] = "0.7"
    additional_param_["max_model_size"] = "20"
    additional_param_["match_icorrs"] = "N"
    additional_param_["cutoff_l1_reg"] = "5"
    additional_param_["change_ratio"] = "0.5"
    additional_param_["num_pca_comp"] = "1"
    additional_param_["pca_heuristic"] = ""

    if regressalgo == "L1_REGRESSION":
        execpath_ = MODELSCRIPTS_DIR + "lad_regression.R"
        if len(sys.argv) > 6:
            additional_param_["cutoff_l1_reg"] = sys.argv[6]
        if len(sys.argv) > 7:
            additional_param_["change_ratio"] = sys.argv[7]
    elif regressalgo == "PCA_REGRESSION":
        execpath_ = MODELSCRIPTS_DIR + "call_pcareg.pl"
        additional_param_["num_pca_comp"] = sys.argv[6]
        additional_param_["max_model_size"] = sys.argv[7]
        additional_param_["pca_heuristic"] = sys.argv[8]
        additional_param_["min_correlation"] = sys.argv[9]
    elif regressalgo == "LASSO":
        execpath_ = MODELSCRIPTS_DIR + "call_lasso.pl"
        additional_param_["max_model_size"] = sys.argv[6]
    elif regressalgo == "SLASSO":
        execpath_ = MODELSCRIPTS_DIR + "call_slasso.pl"
        additional_param_["max_model_size"] = sys.argv[6]
    elif regressalgo == "LR":
        execpath_ = MODELSCRIPTS_DIR + "build_unconstrained_lasso_model.R"
        additional_param_["max_model_size"] = sys.argv[6]
    elif regressalgo == "RIDGE":
        execpath_ = MODELSCRIPTS_DIR + "build_unconstrained_ridge_model.R"
        additional_param_["df"] = sys.argv[6]
        additional_param_["min_variance_explained"] = sys.argv[7]
    elif regressalgo == "LM":
        execpath_ = MODELSCRIPTS_DIR + "build_linear_model.R"
    elif regressalgo == "EARTH":
        execpath_ = LIVE_BIN_DIR + "callMARS"
    elif regressalgo in ["FSLR", "FSHLR", "FSHDVLR"]:
        execpath_ = LIVE_BIN_DIR + "call" + regressalgo
        additional_param_["min_correlation"] = sys.argv[6]
        additional_param_["first_indep_weight"] = sys.argv[7]
        additional_param_["must_include_first_k_independents"] = sys.argv[8]
        additional_param_["max_indep_correlation"] = sys.argv[9]
        additional_param_["max_model_size"] = sys.argv[10]
    elif regressalgo in ["FSRR", "FSRLM", "FSVLR"]:
        execpath_ = LIVE_BIN_DIR + "call" + regressalgo
        additional_param_["regulariser"] = sys.argv[6]
        additional_param_["min_correlation"] = sys.argv[7]
        additional_param_["first_indep_weight"] = sys.argv[8]
        additional_param_["must_include_first_k_independents"] = sys.argv[9]
        additional_param_["max_indep_correlation"] = sys.argv[10]
        additional_param_["max_model_size"] = sys.argv[11]
    elif regressalgo in ["FSRMFSS", "FSRMSH", "FSRSHRSQ", "MULTLR"]:
        if regressalgo == "FSRMFSS":
            execpath_ = LIVE_BIN_DIR + "callfsr_mean_fss_corr"
        elif regressalgo == "FSRMSH":
            execpath_ = LIVE_BIN_DIR + "callfsr_mean_sharpe_corr"
        elif regressalgo == "FSRSHRSQ":
            execpath_ = LIVE_BIN_DIR + "callfsr_sharpe_rsq"
        elif regressalgo == "MULTLR":
            execpath_ = LIVE_BIN_DIR + "call_multiple_fslr"
        additional_param_["regulariser"] = sys.argv[6]
        additional_param_["min_correlation"] = sys.argv[7]
        additional_param_["first_indep_weight"] = sys.argv[8]
        additional_param_["must_include_first_k_independents"] = sys.argv[9]
        additional_param_["max_indep_correlation"] = sys.argv[10]
        additional_param_["max_model_size"] = sys.argv[11]
        additional_param_["match_icorrs"] = sys.argv[12]

    ReadFile(filename_)
    PerformPartition()
    CrossValidation()
    ExtractModel()


_main_()
