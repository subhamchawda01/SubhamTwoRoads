#!/usr/bin/env python

import sys
from scipy.stats import spearmanr
from scipy.stats import pearsonr
import numpy as np
import bisect

if len(sys.argv) < 4:
    print "USAGE:<script><model file><datagen file><sent orders file><executed order file>"
    exit(0)

model_file_ = sys.argv[1]
datagen_file_ = sys.argv[2]
sent_orders_file_ = sys.argv[3]
exec_orders_file_ = sys.argv[4]
CAOS_ = {}
current_exec_time_ = 0
exec_diffs_ = []
exec_times = []

with open(exec_orders_file_) as file:
    for line in file:
        line = line.strip().split()
        trade_exec_mfm_ = int(float(line[0]) % 86400 * 1000)
        exec_times.append(trade_exec_mfm_)
        # trade sent mfm,trade exec mfm,ind values at sent time,base price at sent time,tgt price at exec time
        CAOS_[int(line[1])] = [0, trade_exec_mfm_, [], 0, 0]
file.close()
exec_times.sort()
avg_trade_time_ = np.mean(np.nonzero([abs(j - i) for i, j in zip(exec_times[:-1], exec_times[1:])])) * 1000
with open(sent_orders_file_) as file:
    for line in file:
        line = line.strip().split()
        if int(line[1]) in CAOS_:
            trade_sent_mfm_ = int(float(line[0]) % 86400 * 1000)
            CAOS_[int(line[1])][0] = trade_sent_mfm_
file.close()

indicator_weights_ = []
indicator_string_ = []

with open(model_file_) as file:
    for line in file:
        line = line.strip().split()
        if line[0] == "INDICATOR":
            indicator_weights_.append(float(line[1]))
            indicator_string_.append(' '.join(line[2:]))
file.close()
num_ind_ = len(indicator_weights_)

actual_bias_ = []
trade_bias_ = []
ind_values_ = []
predicted_signs = 0
event_times_ = []
with open(datagen_file_) as file:
    lines = file.readlines()
    lines = [line.strip().split() for line in lines]
    event_times_ = [float(line[0]) for line in lines]
    i = 0
    for caos in sorted(CAOS_):
        while i < len(lines) - 1:
            if (float(lines[i][0]) == CAOS_[caos][0]) or (float(lines[i][0]) < CAOS_[caos][0] and float(lines[i + 1][0]) > CAOS_[caos][0]):
                CAOS_[caos][2] = map(float, lines[i][4:])
                CAOS_[caos][3] = float(lines[i][2])
                break
            else:
                i = i + 1
    for caos in sorted(CAOS_):
        # gotta do n^2 since execution order might be different
        i = 0
        while i < len(lines) - 1:
            if (float(lines[i][0]) == CAOS_[caos][1]) or (float(lines[i][0]) < CAOS_[caos][1] and float(lines[i + 1][0]) > CAOS_[caos][1]):
                begin_idx_ = i
                end_idx_ = min(len(lines), bisect.bisect_left(event_times_, CAOS_[caos][1] + avg_trade_time_))
                # print CAOS_[caos][1]+avg_trade_time_,CAOS_[caos][1],begin_idx_,end_idx_
                CAOS_[caos][4] = np.mean([float(lines[j][3]) for j in xrange(begin_idx_, end_idx_)])
                break
            else:
                i = i + 1

file.close()
for caos in sorted(CAOS_):
    current_ind_values_ = CAOS_[caos][2]
    if len(current_ind_values_) > 0:
        actual_bias_.append(np.dot(current_ind_values_, indicator_weights_))
        trade_bias_.append(CAOS_[caos][4] - CAOS_[caos][3])
        ind_values_.append(np.multiply(current_ind_values_, indicator_weights_))
        if trade_bias_[len(trade_bias_) - 1] * actual_bias_[len(actual_bias_) - 1] > 0:
            predicted_signs = predicted_signs + 1

ind_values_ = np.array(ind_values_)
corr = []
signs = [0 for i in xrange(ind_values_.shape[1])]
significance = []

for i in range(len(trade_bias_)):
    contribution = np.abs(ind_values_[i])
    significance.append((contribution * 100 / np.sum(contribution)).tolist())
significance = np.array(significance)

for i in xrange(ind_values_.shape[1]):
    corr.append([pearsonr(trade_bias_, ind_values_[:, i])[0], spearmanr(trade_bias_, ind_values_[:, i])[0]])
    for j in range(len(trade_bias_)):
        if trade_bias_[j] * ind_values_[j][i] > 0:
            signs[i] = signs[i] + 1

order = np.argsort(np.array(corr)[:, 0])
print "Classification rate :", predicted_signs * 1.0 / len(trade_bias_)
print "Correlation :", "Pearson", pearsonr(actual_bias_, trade_bias_), "Spearman", spearmanr(actual_bias_, trade_bias_)
print "Classification\tPearsonr\tSpearmanr\tImpact factor"
for i in xrange(ind_values_.shape[1]):
    print signs[i] * 1.0 / len(trade_bias_), corr[order[i]][0], corr[order[i]][1], np.mean(significance[:, i]), ':', indicator_string_[order[i]]

#i = 0
# for caos in sorted(CAOS_):
#	print "ID",caos,"Indicator contribution",significance[i]
#	i = i + 1
