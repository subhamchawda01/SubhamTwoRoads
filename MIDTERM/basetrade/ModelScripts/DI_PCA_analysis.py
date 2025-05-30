#!/usr/bin/env python

# this script used sklearn.Run this command before running this script : source/apps/pythonenv/bin/activate

import numpy as np
import sys
from sklearn.decomposition import PCA

# major assumption : the train and test data files have been obtained by smoothening
# script datagen_condense.py brings the raw data file in the relevant format
# use cat $(ls) > datafile to concatenate the datagen files into one single train or test file

if len(sys.argv) < 6:
    print "USAGE:<training file><test file><train inefficiency file><test inefficiency file><allowance factor>[verbose=False]"
    exit(0)

train_file_ = sys.argv[1]
test_file_ = sys.argv[2]
train_ineff_ = sys.argv[3]
test_ineff_ = sys.argv[4]
dev_allowed_ = float(sys.argv[5])
verbose_ = bool(sys.argv[6]) if len(sys.argv) > 6 else False

data = np.loadtxt(train_file_)
pca = PCA()
pca.fit(data)

ineff_second = []
ineff_third = []
ineff_fourth = []
ineff = []

for i in xrange(data.shape[0]):
    ineff.append(data[i] - (np.dot(data[i], pca.components_[0]) * pca.components_[0]))
    ineff_second.append(np.dot(data[i], pca.components_[1]) * pca.components_[1])
    ineff_third.append(np.dot(data[i], pca.components_[2]) * pca.components_[2])
    ineff_fourth.append(np.dot(data[i], pca.components_[3]) * pca.components_[3])

mean_revert_ = np.mean(ineff, axis=0)
allowance_ = dev_allowed_ * np.std(ineff, axis=0)
if verbose_ == True:
    print "For the training data pc1 explains", pca.explained_variance_ratio_[0] * 100, "variance"
    print "mean values of inefficiencies in training", mean_revert_


# by convention we assume that we will be trading in V and N contracts i.e the 2nd and 3rd columns
data_test = np.loadtxt(test_file_)

# risk_vector = pca.components_[1] #/np.min(np.abs(pca.components_[1])) #+ pca.components_[2] + pca.components_[3]
#risk_vector /= np.sum(np.square(risk_vector))
# print risk_vector
# print pca.components_

risk_sum = [0 for i in xrange(data.shape[1])]
trades = [0 for i in xrange(data.shape[1])]
pnl = [0 for i in xrange(data.shape[1])]
positions = [0 for i in xrange(data.shape[1])]
last_price = [0 for i in xrange(data.shape[1])]
last_buy_price = [0 for i in xrange(data.shape[1])]
last_sell_price = [0 for i in xrange(data.shape[1])]

test_ineff = []

for i in xrange(0, data_test.shape[0]):
    bid_price = [data_test[i][3 * j] for j in xrange(data.shape[1])]
    ask_price = [data_test[i][3 * j + 1] for j in xrange(data.shape[1])]
    mkt_price = [data_test[i][3 * j + 2] for j in xrange(data.shape[1])]
    price_transform = mkt_price - (np.dot(mkt_price, pca.components_[0]) * pca.components_[0])
    risk_vector = price_transform / np.sum(np.square(price_transform))
    last_buy_price = ask_price
    last_sell_price = bid_price
    #price_transform = (np.dot(mkt_price,pca.components_[1])*pca.components_[1])
    test_ineff.append(price_transform)
    for j in xrange(data.shape[1]):
        if mean_revert_[j] + allowance_[j] < price_transform[j] and positions[j] > -100:
            pnl[j] = pnl[j] + np.abs(risk_vector[j]) * bid_price[j]
            risk_sum[j] = risk_sum[j] + np.abs(risk_vector[j])
            trades[j] = trades[j] + 1
            positions[j] = positions[j] - np.abs(risk_vector[j])
            last_price[j] = mkt_price[j]
        elif mean_revert_[j] - allowance_[j] > price_transform[j] and positions[j] < 100:
            pnl[j] = pnl[j] - np.abs(risk_vector[j]) * ask_price[j]
            risk_sum[j] = risk_sum[j] + np.abs(risk_vector[j])
            trades[j] = trades[j] + 1
            positions[j] = positions[j] + np.abs(risk_vector[j])
            last_price[j] = mkt_price[j]
    if i != 0 and (i + 1) % 16 == 0 and verbose_ == True:
        print pnl, positions, risk_vector

if verbose_ == True:
    print "mean values of efficiencies in test", np.mean(test_ineff, axis=0)

# close all open positions conservatively
for i in xrange(data.shape[1]):
    print positions[i], last_price[i]
    pnl[i] = pnl[i] + positions[i] * (last_buy_price[i] if positions[i] < 0 else last_sell_price[i])

for i in xrange(data.shape[1]):
    print "pnl for", i, pnl[i] * trades[j] / (0.01 * np.abs(risk_sum[j]))  # the tick value of DI contracts is 0.01
    print "Final open position for", i, positions[i]

np.savetxt(train_ineff_, ineff, fmt='%.6f')
np.savetxt(test_ineff_, test_ineff, fmt='%.6f')
