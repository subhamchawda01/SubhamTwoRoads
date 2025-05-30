#!/usr/bin/env python

import numpy as np
import sys
import os
import pandas as pd

weights_ = []
X = []
Y = []
isLassoType_ = 0
maxAllowedError_ = 1


def ReadFile(filename):
    global X
    global Y
    t_data_ = pd.read_csv(filename, sep=' ').values
    Y = t_data_[:, 0]
    X = t_data_[:, 1:t_data_.shape[1]]


def ObtainWeights(filename):
    global weights_
    global X
    weights_ = np.zeros(X.shape[1])
    if os.path.exists(filename):
        with open(filename) as file:
            for line in file:
                line = line.strip().split()
                if isLassoType_ == 1:
                    idx_ = int(line[0]) - 1
                    weights_[idx_] = float(line[1])
                else:
                    if line[0] == "OutCoeff":
                        idx_ = int(line[1])
                        weights_[idx_] = float(line[2])
        file.close()

# define the performance paramter for non linear models


def MSE():
    global weights_
    global X
    global Y
    Y_predict = np.dot(X, np.transpose(weights_))
    return (np.sum((Y_predict - Y)**2)) / (np.sum((Y - np.mean(Y))**2))


def __main__():
    global isLassoType_
    global maxAllowedError_
    global Y

    if len(sys.argv) < 4:
        print "Usage:<script><LASSO/NONLASSO><modelfile><regdatafile><validation cutoff>"
        exit(1)

    isLassoType_ = int(sys.argv[1])
    ReadFile(sys.argv[3])
    ObtainWeights(sys.argv[2])

    cutoff_ = float(sys.argv[4])
    if cutoff_ == -1:  # choose from the data
        # cutoff_ = np.median(np.abs(Y - np.median(Y))) #work on choosing this value
        cutoff_ = 1.5 * np.std(np.abs(Y - np.median(Y)))
#	print cutoff_,MSE()
    if cutoff_ < MSE():
        print "0", MSE()  # model is not robust
    else:
        print "1", MSE()


__main__()
