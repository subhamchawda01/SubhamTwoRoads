#!/usr/bin/env python

import numpy as np
import pandas as pd
import os
import sys
import subprocess
from sklearn import linear_model, datasets
from sklearn.feature_selection import SelectKBest, f_regression
import math


def ReduceIndicators():
    global X
    global Y
    global number_of_indicators
    number_of_indicators = min(number_of_indicators, X.shape[1])
    #selector = SelectKBest(score_func=f_regression,k=number_of_indicators)
    selector = linear_model.RandomizedLasso(fit_intercept=False)
    selector.fit(X, Y)
    selector.scores_[np.isnan(selector.scores_)] = 0
    return np.sort(selector.scores_.argsort()[-number_of_indicators:][::-1])


def ReadFile():
    global X
    global Y
    global regdatafilename
    t_data_ = pd.read_csv(regdatafilename, sep=' ', header=None).values
    Y = np.array(t_data_[:, 0])
    X = np.array(t_data_[:, 1:t_data_.shape[1]])


def PopulateDateMap():
    global granularity_range
    global dates
    global datefile

    current_begin = 0
    with open(datefile) as file:
        for line in file:
            line = line.strip().split()
            current_end = current_begin + int(line[0])
            #granularity_range[line[1]] = (current_begin,current_end-1)
            dates.append(line[1])
            granularity_range.append((current_begin, current_end - 1))
            current_begin = current_end
    file.close()


def SingleNormalSimIteration(model):

    global weights
    global granularity_range
    global modelfilename
    global dates
    global stratfilename
    global paramfilename
    global product
    global strategy
    global starttime
    global endtime
    global filewritedata
    global modifierlines

    training_granular_ = np.unique(np.random.choice(len(granularity_range), len(granularity_range)))
    training_idx_ = []
    for idx in training_granular_:
        training_idx_.extend(list(xrange(granularity_range[idx][0], granularity_range[idx][1] + 1)))
    model.fit(X[training_idx_], Y[training_idx_])
    loss = np.zeros(len(granularity_range),)

    # write to model file
    for idx in xrange(model.coef_.shape[0]):
        filewritedata[modifierlines[idx]][1] = str(model.coef_[idx])
    file_write = open(modelfilename, 'w+')
    for item in filewritedata:
        file_write.write(" ".join(item) + "\n")
    file_write.close()

    # obtain the pnls
    output = np.zeros(len(granularity_range),)
    cores = float(subprocess.Popen("nproc", shell=True, stdout=subprocess.PIPE).communicate()[0].rstrip('\n'))
    last_5_min_core_normalised_avg = float(subprocess.Popen(
        "uptime | grep \"load average\" | awk '{print $11}'", shell=True, stdout=subprocess.PIPE).communicate()[0].rstrip('\n').rstrip(',')) / cores
    parallel_process_ = max(1, int((1.3 - last_5_min_core_normalised_avg) * cores))
    parallel_size_ = min(len(granularity_range), parallel_process_)
    current_begin = 0
    while current_begin < len(granularity_range):
        current_end = min(current_begin + parallel_size_, len(granularity_range))
        processes = [subprocess.Popen("~/LiveExec/bin/sim_strategy" + " SIM " + stratfilename + " 101313 " + date +
                                      " 2>/dev/null", shell=True, stdout=subprocess.PIPE, bufsize=1) for date in dates[current_begin:current_end]]
        output[current_begin:current_end] = [
            float(process.communicate()[0].strip().split()[1]) for process in processes]
        current_begin = current_begin + parallel_size_

    # normalise the losses and propagate
    loss = np.divide(1., 2 * (1 + np.exp(0.0001 * output)))
    avg_loss_ = (weights * loss).sum()
    single_beta_ = avg_loss_ / ((1 - avg_loss_))
    weights *= np.power(single_beta_, 1. - loss)
    weights /= np.sum(weights)
    return model, single_beta_, avg_loss_


def SingleSimpleSimIteration(model):

    global weights
    global granularity_range
    global dates
    global paramfilename
    global timestamplist
    global date_to_mktdata
    global product

    training_granular_ = np.unique(np.random.choice(len(granularity_range), len(granularity_range)))
    training_idx_ = []
    for idx in training_granular_:
        training_idx_.extend(list(xrange(granularity_range[idx][0], granularity_range[idx][1] + 1)))
    model.fit(X[training_idx_], Y[training_idx_])
    loss = np.zeros(len(granularity_range),)
    Y_predict = model.predict(X)

    # obtain the pnls
    output = np.zeros(len(granularity_range),)
    parallel_size_ = min(len(granularity_range), 30)
    current_begin = 0
    idx = 0
    while current_begin < len(granularity_range):
        current_end = min(current_begin + parallel_size_, len(granularity_range))
        # prepare the reg data file
        for date in dates[current_begin:current_end]:
            range_ = granularity_range[idx]
            idx = idx + 1
            np.savetxt("reg_data_" + date,
                       zip(timestamplist[range_[0]:range_[1] + 1], Y_predict[range_[0]:range_[1] + 1]), fmt="%.10f")
        filenames = ["reg_data_" + date for date in dates[current_begin:current_end]]
        processes = [subprocess.Popen("~/basetrade/ModelScripts/simple_sim_strategy.py" + " " + product + " " + date + " " + date_to_mktdata[date] + " " + file + " " +
                                      paramfilename + " 2>/dev/null | tail -n1", shell=True, stdout=subprocess.PIPE, bufsize=1) for file, date in zip(filenames, dates[current_begin:current_end])]
        output[current_begin:current_end] = [
            float(process.communicate()[0].strip().split()[1]) for process in processes]
        current_begin = current_begin + parallel_size_
        [os.remove("reg_data_" + date) for date in dates[current_begin:current_end]]

    # normalise the losses and propagate
    loss = np.divide(1., 2 * (1 + np.exp(0.0001 * output)))
    avg_loss_ = (weights * loss).sum()
    single_beta_ = avg_loss_ / ((1 - avg_loss_))
    weights *= np.power(single_beta_, 1. - loss)
    weights /= np.sum(weights)
    return model, single_beta_, avg_loss_


def TrainWithBoosting(model):
    global weights
    global alpha_RR
    global granularity_range
    global runSimpleSim

    weights = np.repeat(1.0 / len(granularity_range), len(granularity_range))
    if runSimpleSim == False:
        t_model, t_beta, t_avg_loss_ = SingleNormalSimIteration(model)
    else:
        t_model, t_beta, t_avg_loss_ = SingleSimpleSimIteration(model)
    steps = 1
    while t_avg_loss_ < 0.5 and steps < 50:
        ensemble.append(t_model)
        beta.append(t_beta)
        model = linear_model.Ridge(alpha=alpha_RR, normalize=True)
        if runSimpleSim == False:
            t_model, t_beta, t_avg_loss_ = SingleNormalSimIteration(model)
        else:
            t_model, t_beta, t_avg_loss_ = SingleSimpleSimIteration(model)
        steps = steps + 1
    return steps


def PrintModelFile(filename):
    lines = open(filename).read().splitlines()
    split_index = lines.index("INDICATORSTART")
    print lines[0]
    print "MODELMATH BOOSTING CHANGES"
    for i in range(2, split_index):
        print lines[i]
    lines = lines[split_index + 1:len(lines) - 1]
    for i in range(len(ensemble)):
        if i == 0:
            print "INDICATORSTART", math.log(1 / beta[i])
        else:
            print "INDICATORINTERMEDIATE", math.log(1 / beta[i])
        model_idx_ = 0
        for j in range(len(lines)):  # print each indicator value
            if j in idx_to_consider:
                if ensemble[i].coef_[model_idx_] != 0:
                    line = lines[j].strip().split()
                    line[1] = str(ensemble[i].coef_[model_idx_])
                    print " ".join(line)
                model_idx_ = model_idx_ + 1
    print "INDICATOREND"


def PredictWithMedian(X):
    predictions = []
    Ybar = []
    for i in range(len(ensemble)):
        predictions.append(ensemble[i].predict(X))
    sort_idx_ = np.argsort(np.array(predictions), axis=0)
    sum_total_log_beta_ = np.sum(np.log(np.divide(1.0, beta)))
    for i in range(X.shape[0]):
        sum_total_log_beta_ = 0
        sum_log_beta_ = math.log(1.0 / beta[sort_idx_[0][i]])
        current_idx_ = 0
        while sum_log_beta_ < 0.5 * sum_total_log_beta_:
            current_idx_ = current_idx_ + 1
            sum_log_beta_ = sum_log_beta_ + math.log(1.0 / beta[sort_idx_[current_idx_][i]])
        if current_idx_ != 0:
            Ybar.append(predictions[sort_idx_[current_idx_ - 1][i]][i])
        else:
            Ybar.append(predictions[0][i])
    return np.array(Ybar)


def get_lineage(tree):
    left = tree.tree_.children_left
    right = tree.tree_.children_right
    threshold = tree.tree_.threshold
    features = tree.tree_.feature
    values = tree.tree_.value
    # get ids of child nodes
    bfs = []
    bfs.append(0)
    while(len(bfs) > 0):
        parent = bfs[0]
        leftchild = left[parent]
        rightchild = right[parent]
        indicator_index = features[parent]
        thres = threshold[parent]
        if leftchild == -1 and rightchild == -1:
            leaf = 'Y'
        else:
            leaf = 'N'
        val = values[parent][0][0]
        bfs.pop(0)
        if(rightchild != -1):
            bfs.insert(0, rightchild)
        if(leftchild != -1):
            bfs.insert(0, leftchild)
        if indicator_index < 0:
            indicator_index = -1
        if thres == -2.0:
            thres = 0
        print "TREELINE", leftchild, rightchild, indicator_index, thres, leaf, val


def returnScore(Xbar, Ybar):
    #	print "Boosting completed in ",str(steps)
    sse = np.sum((PredictWithMedian(Xbar) - Ybar)**2)
    sst = np.sum((Ybar - np.average(Ybar))**2)
    return 1 - sse / sst


if len(sys.argv) < 13:
    print "Usage:<script><regdatafilename><datefile><alpha_RR><max_indicators_size><ilistfilename><modelfilename><paramfilename><stratfilename><starttime><endtime><product><strategy>[timestampfile][mktdataloggerfilelist]"
    exit(1)

# for granularity n the day will be divided into roughly n equal parts
X = []
Y = []

weights = []
beta = []
ensemble = []
granularity_range = []
dates = []
granularity_wise_loss_ = []

regdatafilename = sys.argv[1]
datefile = sys.argv[2]
alpha_RR = float(sys.argv[3])
number_of_indicators = int(sys.argv[4])
ilistfilename = sys.argv[5]
modelfilename = sys.argv[6]
paramfilename = sys.argv[7]
stratfilename = sys.argv[8]
starttime = sys.argv[9]
endtime = sys.argv[10]
product = sys.argv[11]
strategy = sys.argv[12]
timestampfile = ""
mktdataloggerfilelist = ""
date_to_mktdata = {}
timestamplist = []
runSimpleSim = False

if len(sys.argv) > 13:
    if len(sys.argv) == 15:
        timestampfile = sys.argv[13]
        mktdataloggerfilelist = sys.argv[14]
        timestamplist = pd.read_csv(timestampfile, sep=' ', header=None).values
        with open(mktdataloggerfilelist) as file:
            for line in file:
                line = line.strip().split()
                date_to_mktdata[line[0]] = line[1]
            file.close()
        runSimpleSim = True
    else:
        print "must specify both timestamp file and mkt data logger file list"
        exit(1)

ReadFile()
PopulateDateMap()

idx_to_consider = np.array(ReduceIndicators())
X = X[:, idx_to_consider]

# create model file
filewritedata = []
modifierlines = []

file_read = open(ilistfilename, 'r')
indicator_idx_ = 0
line_counter = 0


for line in file_read:
    entails = line.strip().split()
    if entails[0] == "INDICATOR":
        if indicator_idx_ in idx_to_consider:
            filewritedata.append(entails)
            modifierlines.append(line_counter)
            line_counter = line_counter + 1
        indicator_idx_ = indicator_idx_ + 1
    else:
        filewritedata.append(entails)
        line_counter = line_counter + 1

file_read.close()

# create strat file
strat_file = open(stratfilename, 'w+')
strat_file.write("STRATEGYLINE " + product + " " + strategy + " " + modelfilename +
                 " " + paramfilename + " " + starttime + " " + endtime + " 101313\n")
strat_file.close()

model = linear_model.Ridge(alpha=alpha_RR, normalize=True)
steps = TrainWithBoosting(model)
PrintModelFile(ilistfilename)

print "RSquared", returnScore(X, Y)  # for cross validation
