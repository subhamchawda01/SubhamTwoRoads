#!/usr/bin/env python

import os
import sys
import math
import subprocess
import numpy as np


def call_FSRR(fsrr_args, ilist, reg_data, reg_out, avoid_high_sharpe_indep_check_index_filename_):
    fsrr_script = "/home/dvctrader/LiveExec/bin/callFSRR"
    tokens = fsrr_args.split()
    reg_coff = tokens[1]
    reg_out_temp = reg_out + "_temp"
    min_corr = tokens[2]
    first_indep_weight = tokens[3]
    must_include_first_k_independents = tokens[4]
    max_indep_corr = tokens[5]
    max_model_size = 12
    if (len(tokens) > 6):
        max_model_size = tokens[6]
    match_icorrs = "N"
    if len(tokens) > 7:
        match_icorrs = tokens[7]
    cmd = fsrr_script + " " + reg_data + " " + reg_coff + " " + min_corr + " " + first_indep_weight + " " + must_include_first_k_independents + \
        " " + max_indep_corr + " " + reg_out_temp + " " + max_model_size + " " + avoid_high_sharpe_indep_check_index_filename_
    if match_icorrs == "I":
        cmd += " " + ilist
    if len(tokens) > 8:
        cmd += " " + tokens[8]
    if len(tokens) > 9:
        cmd += " " + tokens[9]
    os.system(cmd)

    place_coeffs_script = "/home/dvctrader/basetrade_install/ModelScripts/place_coeffs_in_model.pl"
    cmd = place_coeffs_script + " " + reg_out + " " + ilist + " " + reg_out_temp
    os.system(cmd)


def combine_models(model1, model2, pred_dur1, pred_dur2, pred_dur, model):
    w1 = 0.5
    w2 = 0.5
    if pred_dur < pred_dur1:
        w1 = 0.5 * math.sqrt(1.0 * pred_dur / pred_dur1)
    if pred_dur < pred_dur2:
        w2 = 0.5 * math.sqrt(1.0 * pred_dur / pred_dur2)
    weight_map = {}
    f = open(model1, 'r')
    for line in f:
        tokens = line.split()
        if tokens[0] == "INDICATOR":
            end_indx = len(tokens)
            for j in range(len(tokens)):
                if tokens[j][0] == '#':
                    end_indx = j
                    break
            weight_map[" ".join(x for x in tokens[2:end_indx])] = w1 * float(tokens[1])
    f.close()

    f = open(model2, 'r')
    for line in f:
        tokens = line.split()
        if tokens[0] == "INDICATOR":
            end_indx = len(tokens)
            for j in range(len(tokens)):
                if tokens[j][0] == '#':
                    end_indx = j
                    break
            indicator = " ".join(x for x in tokens[2:end_indx])
            if indicator in weight_map:
                weight_map[indicator] += w2 * float(tokens[1])
            else:
                weight_map[indicator] = w2 * float(tokens[1])
    f.close()

    f = open(model, 'w')
    g = open(model1, 'r')
    s = g.readlines()
    f.writelines(s[0:3])
    for key in weight_map.keys():
        f.write("INDICATOR " + str(weight_map[key]) + " " + key + "\n")
    f.write("INDICATOREND")
    f.close()


regdata_filename = sys.argv[1]
pred_dur = float(sys.argv[2])
config = sys.argv[3]
ilist = sys.argv[4]
pred_dur1 = float(sys.argv[5])
pred_dur1 = pred_dur1 / 1000
pred_dur2 = float(sys.argv[6])
pred_dur2 = pred_dur2 / 1000
reg_out = sys.argv[7]
avoid_high_sharpe_indep_check_index_filename_ = sys.argv[8]


regdata = np.loadtxt(regdata_filename)

f = open(config, 'r')
config_lines = f.readlines()
f.close()

f = open(ilist, 'r')
indicators = f.readlines()
f.close()

reg1 = config_lines[0]
reg2 = config_lines[1]
reg1 = reg1.strip()
reg2 = reg2.strip()
reg_out_low = reg_out + "_low"
reg_out_high = reg_out + "_high"

ilist_low = ilist + "_low"
ilist_high = ilist + "_high"
w1 = open(ilist_low, 'w')
w2 = open(ilist_high, 'w')
w1.writelines(indicators[0:3])
w2.writelines(indicators[0:3])
regdata_filename_low = regdata_filename + "_low"
regdata_filename_high = regdata_filename + "_high"
ind1 = [0]
ind2 = [1]
for i in range(3, len(indicators) - 1):
    indicator = indicators[i]
    index = indicator.find('#')
    if index == -1:
        w1.write(indicator)
        ind1.append(i - 1)
        w2.write(indicator)
        ind2.append(i - 1)
    else:
        right_half = indicator[index + 1:]
        right_half = right_half.strip()
        if right_half[0] == 'S':
            w1.write(indicator)
            ind1.append(i - 1)
        elif right_half[0] == 'L':
            w2.write(indicator)
            ind2.append(i - 1)
        else:
            w1.write(indicator)
            ind1.append(i - 1)
            w2.write(indicator)
            ind2.append(i - 1)
w1.write("INDICATOREND")
w2.write("INDICATOREND")
w1.close()
w2.close()
regdata_low = regdata[:, ind1]
regdata_high = regdata[:, ind2]
np.savetxt(regdata_filename_low, regdata_low, fmt='%5.6f')
np.savetxt(regdata_filename_high, regdata_high, fmt='%5.6f')
call_FSRR(reg1, ilist_low, regdata_filename_low, reg_out_low, avoid_high_sharpe_indep_check_index_filename_)
call_FSRR(reg2, ilist_high, regdata_filename_high, reg_out_high, avoid_high_sharpe_indep_check_index_filename_)
combine_models(reg_out_low, reg_out_high, pred_dur1, pred_dur2, pred_dur, reg_out)
