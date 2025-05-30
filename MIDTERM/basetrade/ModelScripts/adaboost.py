#!/usr/bin/env python

import numpy as np
import pandas as pd
import sys
from sklearn import linear_model, datasets
from sklearn.tree import DecisionTreeRegressor
import math
from sklearn.feature_selection import SelectKBest, f_regression


def ReduceIndicators():
    global X
    global Y
    number_of_indicators = int(sys.argv[5])
    #selector = SelectKBest(score_func=f_regression,k=number_of_indicators)
    selector = linear_model.RandomizedLasso(fit_intercept=False)
    selector.fit(X, Y)
    selector.scores_[np.isnan(selector.scores_)] = 0
    return np.sort(selector.scores_.argsort()[-number_of_indicators:][::-1])


def ReadFile():
    global X
    global Y
    t_data_ = pd.read_csv(sys.argv[2], sep=' ', header=None).values
    Y = np.array(t_data_[:, 0])
    X = np.array(t_data_[:, 1:t_data_.shape[1]])


def random_partition(n, n_data):
    all_idxs = np.arange(n_data)
    np.random.shuffle(all_idxs)
    idxs1 = all_idxs[:n]
    idxs2 = all_idxs[n:]
    return idxs1, idxs2


def SingleIteration(model):
    global weights
    training_idx_ = []
    weights /= np.sum(weights)  # just in case the weights do not add up to 1
    training_idx_ = np.unique(np.random.choice(X.shape[0], X.shape[0], p=weights))

    model.fit(X[training_idx_], Y[training_idx_])
    Y_predict = model.predict(X)
    loss = np.abs(Y_predict - Y)
    D = loss.max()
    if D != 0:
        loss /= D
    if loss_fn == "Square":
        loss **= 2
    elif loss_fn == "Exponential":
        loss = 1. - np.exp(-loss)
    avg_loss_ = (weights * loss).sum()
    single_beta_ = avg_loss_ / (1 - avg_loss_)
    weights *= np.power(single_beta_, 1. - loss)

    return model, single_beta_, avg_loss_


def TrainWithBoosting(model, loss_threshold):
    global weights
    weights = np.repeat(1.0 / X.shape[0], X.shape[0])
    t_model, t_beta, t_avg_loss_ = SingleIteration(model)
    steps = 1
    while t_avg_loss_ < loss_threshold and steps < 50:
        ensemble.append(t_model)
        beta.append(t_beta)
        if sys.argv[1] == "BOOSTING":
            model = linear_model.Ridge(alpha=float(sys.argv[3]), normalize=True)
        else:
            model = DecisionTreeRegressor(splitter='random', max_depth=int(sys.argv[3]))
        t_model, t_beta, t_avg_loss_ = SingleIteration(model)
        steps = steps + 1
    return steps


def PrintModelFileLinear(filename):
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


def PrintModelFileTree(filename):
    lines = open(filename).read().splitlines()
    split_index = lines.index("INDICATORSTART")
    print lines[0]
    print "MODELMATH TREEBOOSTING CHANGES"
    for i in range(2, len(lines)):
        if lines[i][0:10] == "INDICATOR ":
            if i - split_index - 1 in idx_to_consider:
                print lines[i]
        else:
            print lines[i]
    for i in range(len(ensemble)):
        print "TREESTART", math.log(1 / beta[i])
        get_lineage(ensemble[i])
    print "ENSEMBLEEND"


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


if len(sys.argv) < 7:
    print "Usage:<script><TREEBOOSTING/BOOSTING><regdatafilename><max_depth/alpha_RR><loss_function><max_indicators_size><modelfilename>[CVTest][max_allowed_error][statusfile]"
    exit(1)

X = []
Y = []

weights = []
beta = []
ensemble = []
loss_fn = sys.argv[4]

ReadFile()

idx_to_consider = np.array(list(xrange(X.shape[1])))
if(X.shape[1] > int(sys.argv[5])):
    idx_to_consider = np.array(ReduceIndicators())
X = X[:, idx_to_consider]

if sys.argv[1] == "TREEBOOSTING":
    model = DecisionTreeRegressor(splitter='random', max_depth=int(sys.argv[3]))
    model.fit(X, Y)
    steps = TrainWithBoosting(model, 0.5)
    PrintModelFileTree(sys.argv[6])
else:
    model = linear_model.Ridge(alpha=float(sys.argv[3]), normalize=True)
    model.fit(X, Y)
    steps = TrainWithBoosting(model, 0.5)
    PrintModelFileLinear(sys.argv[6])
print "RSquared", returnScore(X, Y)  # for cross validation
if len(sys.argv) > 7:  # evaulate the model
    t_data_ = pd.read_csv(sys.argv[7], sep=' ').values
    test_error_ = 1 - returnScore(t_data_[:, idx_to_consider + 1], t_data_[:, 0])
    status_ = "1"
    if test_error_ > float(sys.argv[8]):
        status_ = "0"
    with open(sys.argv[9], "w+") as statusfile:
        statusfile.write(status_ + "\n")
        statusfile.write("Test error " + str(test_error_) + "\n")
        statusfile.close()
