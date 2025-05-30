#!/usr/bin/env python

import sys
import numpy as np
import pandas as pd
from sklearn.ensemble import AdaBoostClassifier
from sklearn.ensemble import GradientBoostingClassifier
from sklearn.ensemble import RandomForestClassifier
from sklearn.tree import DecisionTreeClassifier
from sklearn.cross_validation import StratifiedShuffleSplit
from sklearn.naive_bayes import GaussianNB
import math

if len(sys.argv) < 3:
    print "USAGE:<script><training features file><training values file><forecast features file>"
    exit(1)

training_X_file = sys.argv[1]
training_Y_file = sys.argv[2]
test_X_file = sys.argv[1]

training_X = np.array(pd.read_csv(training_X_file, header=None, sep=' ').values)
training_Y = np.array(pd.read_csv(training_Y_file, header=None, sep=' ').values)
test_X = np.array(pd.read_csv(test_X_file, header=None, sep=' ').values)

training_data = np.column_stack((training_X, training_Y))
buckets = [[] for i in xrange(5)]
# discretise the space
for i in xrange(training_data.shape[1] - 1, training_data.shape[1]):
    avg = np.mean(training_data[:, i])
    stdev = np.std(training_data[:, i])
    for j in xrange(training_data.shape[0]):
        if training_data[j][i] < avg - 0.5 * stdev:
            buckets[0].append(training_data[j][i])
            training_data[j][i] = 0
        elif training_data[j][i] < avg - 0.25 * stdev:
            buckets[1].append(training_data[j][i])
            training_data[j][i] = 1
        elif training_data[j][i] > avg + 0.5 * stdev:
            buckets[4].append(training_data[j][i])
            training_data[j][i] = 4
        elif training_data[j][i] > avg + 0.25 * stdev:
            buckets[3].append(training_data[j][i])
            training_data[j][i] = 3
        else:
            buckets[2].append(training_data[j][i])
            training_data[j][i] = 2

classes = [i for i in xrange(5)]
pnl = [0 for i in xrange(5)]
for i in xrange(5):
    if len(buckets[i]) > 0:
        pnl[i] = np.sum(buckets[i]) / (1.0 * len(buckets[i]))

#classes = np.sort(np.unique(training_data[:,-1])).tolist()
distr = [0 for i in xrange(len(classes))]
for cls in training_data[:, -1]:
    distr[classes.index(int(cls))] = distr[classes.index(int(cls))] + 1
# print "Class distribution"
# print distr
penalty = [[] for i in xrange(len(classes))]
for i in xrange(len(classes)):
    self_class = int(classes[i])
    for j in xrange(len(classes)):
        penalty[i].append(1.0 / (distr[i]**2) * abs(j - i)) if distr[i] != 0 else penalty[i].append(100000)


penalty = np.array(penalty)
penalty /= np.max(penalty)
# print "Penalty matrix"
# print penalty
#sss = StratifiedShuffleSplit(training_data[:,-1],n_iter=1,test_size=0.25)

# for trn,tst in sss:
#	[train,test] = [training_data[trn],training_data[tst]]


# SAMME.R algorithm
M = 20
weights = np.array([1.0 / training_data.shape[0] for i in xrange(training_data.shape[0])])
alphas = []
models = []

for m in xrange(M):
    model = DecisionTreeClassifier(splitter="random", max_depth=8)
    training_idx_ = np.unique(np.random.choice(training_data.shape[0], training_data.shape[0], p=weights))
    model.fit(training_data[training_idx_, :-1], training_data[training_idx_, -1], sample_weight=weights[training_idx_])
    Y_predict = model.predict(training_data[:, :-1])
    loss = []
    for i in xrange(training_data.shape[0]):
        actual = training_data[i][-1]
        predicted = Y_predict[i]
        loss.append(penalty[classes.index(int(actual))][classes.index(int(predicted))])
    loss = np.array(loss)
    #loss = np.array(loss)/max(loss)
    avg_loss = np.sum(weights * loss) / np.sum(weights)
    if avg_loss == 0 or avg_loss >= 1:
        break
    alpha = math.log((1.0 - avg_loss) / avg_loss) + math.log(len(classes) - 1)
    # print alpha,math.log(len(classes)-1),avg_loss
    weights *= np.exp(alpha * loss)
    weights /= np.sum(weights)
    alphas.append(alpha)
    models.append(model)


#pred = []
# for i in xrange(test.shape[0]):
#	votes = [0 for j in xrange(len(classes))]
#	for j in xrange(len(models)):
#		prediction = int(models[j].predict(test[i][:-1]))
#		votes[classes.index(prediction)] = votes[classes.index(prediction)] + alphas[j]
#	pred.append(votes.index(max(votes)))

#mis = [[0 for i in xrange(len(classes))] for i in xrange(len(classes))]

# for i in xrange(len(pred)):
#	mis[classes.index(int(test[i,-1]))][classes.index(int(pred[i]))] = mis[classes.index(int(test[i,-1]))][classes.index(int(pred[i]))] + 1

# print "Test confusion matrix"
# for i in xrange(len(mis)):
#	print mis[i]

votes = [0 for j in xrange(len(classes))]
for j in xrange(len(models)):
    prediction = int(models[j].predict(test_X)[0])
    votes[classes.index(prediction)] = votes[classes.index(prediction)] + alphas[j]
votes = np.array(votes)
votes /= np.sum(votes)

# print np.dot(votes,np.array([i for i in xrange(votes.shape[0])]))
print np.dot(votes, np.array(pnl))
# print "Class prediction probailities for strat : ",votes/np.sum(votes)
# print "Prediction guide : [0:vb,1:b,2:n,3:g,4:vg]"
