#!/usr/bin/env python

import sys
import numpy as np
import pandas as pd
from scipy.stats import pearsonr
from scipy.stats import spearmanr
from sklearn.ensemble import AdaBoostClassifier
from sklearn.ensemble import GradientBoostingClassifier
from sklearn.ensemble import RandomForestClassifier
from sklearn.tree import DecisionTreeClassifier
from sklearn.cross_validation import StratifiedShuffleSplit
from sklearn.naive_bayes import GaussianNB
import math

if len(sys.argv) < 2:
    print "USAGE:<script><training features file>"
    exit(1)

training_X_file = sys.argv[1]
#training_Y_file = sys.argv[2]

training_X = np.array(pd.read_csv(training_X_file, header=None, delim_whitespace=True).values)
#training_Y = np.array(pd.read_csv(training_Y_file,header=None,sep=' ').values)

training_data = training_X
#training_data = np.column_stack((training_X,training_Y))
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

print "Pnl Buckets"
print(pnl)

#classes = np.sort(np.unique(training_data[:,-1])).tolist()
distr = [0 for i in xrange(len(classes))]
for cls in training_data[:, -1]:
    distr[classes.index(int(cls))] = distr[classes.index(int(cls))] + 1
print "Class distribution"
print distr

penalty = [[] for i in xrange(len(classes))]
for i in xrange(len(classes)):
    self_class = int(classes[i])
    for j in xrange(len(classes)):
        penalty[i].append(1.0 / (distr[i]**2) * abs(j - i)) if distr[i] != 0 else penalty[i].append(100000)


penalty = np.array(penalty)
penalty /= np.max(penalty)
print "Penalty matrix"
print penalty

sss = StratifiedShuffleSplit(training_data[:, -1], n_iter=4, test_size=0.25)
print "Spearman corelation of features with PNL class"
for i in xrange(1, training_data.shape[1] - 1):
    print spearmanr(training_data[:, i], training_data[:, -1])
print "Pearsonr corelation of features with PNL class"
idx_to_consider = []
for i in xrange(1, training_data.shape[1] - 1):
    pcf = pearsonr(training_data[:, i], training_data[:, -1])
    if pcf[1] <= 0.05:
        idx_to_consider.append(i)
    print pearsonr(training_data[:, i], training_data[:, -1])
training_data = training_data[:, 1:]
'''if len(idx_to_consider) > 0 :
	idx_to_consider.append(training_data.shape[1]-1)
	training_data = training_data[:,idx_to_consider]
else:
	print "ERROR : no feature is robust enough to proceed for analysis!!"
	exit(0)'''

train_miss = []
test_miss = []

for trn, tst in sss:
    [train, test] = [training_data[trn], training_data[tst]]

    # SAMME.R algorithm
    M = 10
    weights = np.array([1.0 / train.shape[0] for i in xrange(train.shape[0])])
    alphas = []
    models = []

    for m in xrange(M):
        #model = DecisionTreeClassifier(criterion="entropy",splitter="random",max_depth=max(1,int(np.log(len(idx_to_consider)))))
        model = DecisionTreeClassifier(max_depth=8)
        training_idx_ = np.unique(np.random.choice(train.shape[0], train.shape[0], p=weights))
        # model.fit(train[training_idx_,:-1],train[training_idx_,-1],sample_weight=weights[training_idx_])
        # print model.feature_importances_
        model.fit(train[training_idx_, :-1], train[training_idx_, -1])
        Y_predict = model.predict(train[:, :-1])
        loss = []
        for i in xrange(train.shape[0]):
            actual = train[i][-1]
            predicted = Y_predict[i]
            loss.append(penalty[classes.index(int(actual))][classes.index(int(predicted))])
        loss = np.array(loss)
        avg_loss = np.sum(weights * loss) / np.sum(weights)
        if avg_loss == 0 or avg_loss >= 1:
            break
        alpha = math.log((1.0 - avg_loss) / avg_loss) + math.log(len(classes) - 1)
        weights *= np.exp(alpha * loss)
        weights /= np.sum(weights)
        alphas.append(alpha)
        models.append(model)

    train_pred = []
    train_loss = []
    for i in xrange(train.shape[0]):
        votes = [0 for j in xrange(len(classes))]
        for j in xrange(len(models)):
            prediction = int(models[j].predict(train[i][:-1]))
            votes[classes.index(prediction)] = votes[classes.index(prediction)] + alphas[j]
        actual = train[i][-1]
        predicted = votes.index(max(votes))
        votes = np.array(votes)
        pnl_voted = np.mean(votes)
        train_pred.append(predicted)
        train_loss.append(penalty[classes.index(int(actual))][classes.index(int(predicted))])

    test_pred = []
    test_loss = []
    for i in xrange(test.shape[0]):
        votes = [0 for j in xrange(len(classes))]
        for j in xrange(len(models)):
            prediction = int(models[j].predict(test[i][:-1]))
            votes[classes.index(prediction)] = votes[classes.index(prediction)] + alphas[j]
        actual = test[i][-1]
        predicted = votes.index(max(votes))
        votes = np.array(votes)
        pnl_voted = np.mean(votes)
        test_pred.append(predicted)
        test_loss.append(penalty[classes.index(int(actual))][classes.index(int(predicted))])

    train_mis = [[0 for i in xrange(len(classes))] for i in xrange(len(classes))]
    train_loss = []
    train_discrete_loss = 0
    train_random_loss = 0
    for i in xrange(len(train_pred)):
        train_mis[classes.index(int(train[i, -1]))][classes.index(int(train_pred[i]))
                                                    ] = train_mis[classes.index(int(train[i, -1]))][classes.index(int(train_pred[i]))] + 1
        train_loss.append(penalty[int(train[i, -1])][classes.index(int(train_pred[i]))])
        if abs(train[i, -1] - train_pred[i]) > 1:
            train_discrete_loss = train_discrete_loss + 1
        if abs(train[i, -1] - np.random.randint(0, 5)) > 1:
            train_random_loss = train_random_loss + 1

    test_mis = [[0 for i in xrange(len(classes))] for i in xrange(len(classes))]
    test_loss = []
    test_discrete_loss = 0
    test_random_loss = 0
    for i in xrange(len(test_pred)):
        test_mis[classes.index(int(test[i, -1]))][classes.index(int(test_pred[i]))
                                                  ] = test_mis[classes.index(int(test[i, -1]))][classes.index(int(test_pred[i]))] + 1
        test_loss.append(penalty[int(test[i, -1])][classes.index(int(test_pred[i]))])
        if abs(test[i, -1] - test_pred[i]) > 1:
            test_discrete_loss = test_discrete_loss + 1
        if abs(test[i, -1] - np.random.randint(0, 5)) > 1:
            test_random_loss = test_random_loss + 1

    print "Train confusion matrix"
    for i in xrange(len(train_mis)):
        print train_mis[i]
    print "Train Average penalty incurred:", np.mean(train_loss)
    print "Train Miss rate:", train_discrete_loss * 1.0 / len(train_pred)
    print "Train Random error rate:", train_random_loss * 1.0 / len(train_pred)
    train_miss.append(train_discrete_loss * 1.0 / len(train_pred))

    print "Test confusion matrix"
    for i in xrange(len(test_mis)):
        print test_mis[i]
    print "Test Average penalty incurred:", np.mean(test_loss)
    print "Test Miss rate:", test_discrete_loss * 1.0 / len(test_pred)
    print "Test Random error rate:", test_random_loss * 1.0 / len(test_pred)
    test_miss.append(test_discrete_loss * 1.0 / len(test_pred))

print "Average test miss", np.mean(test_miss)
print "Average train miss", np.mean(train_miss)
