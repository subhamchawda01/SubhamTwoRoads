#!/usr/bin/python
import sys
import os
import pdb
import subprocess

import pandas as pd
import numpy as np
from scipy import optimize


arg_list = list(sys.argv[0:])
# print arg_list


def get_weightage(df, dep_index, indep_index, sign_vector):
    dep_vector = df[dep_index]
    Indep = df[indep_index]

    for j in range(len(sign_vector)):
        if sign_vector[j] == 0:
            sign_vector[j] = np.sign(np.corrcoef(dep_vector, Indep[indep_index[j]])[0, 1])
        if sign_vector[j] == -1:
            Indep[indep_index[j]] = -Indep[indep_index[j]]
    fit = optimize.nnls(Indep, dep_vector)
    weightage_vector = fit[0]
    weights = [sign_vector[i] * weightage_vector[i] for i in range(len(sign_vector))]
    return weights


def is_non_zero_file(fpath):
    return True if os.path.isfile(fpath) and os.path.getsize(fpath) > 10 else False


if len(arg_list) < 4:
    print("USAGE index_file catted_file output_file")
    sys.exit(0)

index_file = arg_list[1]
catted_file = (arg_list[2])
output_file = (arg_list[3])
df = pd.read_csv(catted_file, sep=" ", header=None)

with open(index_file) as f:
    index_lines = f.readlines()
f.close()
out_file = open(output_file, 'w')

for i in range(len(index_lines)):
    index_line_list = index_lines[i].strip().strip('\n').split(' ')
    if len(index_line_list) < 4:
        continue
    portfolio = index_line_list[0]
    dep_index = int(index_line_list[1])
    indep_index = []
    sign_vector = []
    for i in range(2, len(index_line_list) - 1, 2):
        indep_index.append(int(index_line_list[i]))
        if index_line_list[i + 1] == "+" or index_line_list[i + 1] == "-":
            sign_vector.append(int(index_line_list[i + 1] + "1"))
        elif index_line_list[i + 1] == "0":
            sign_vector.append(0)
        else:
            print("expected + -  or 0 as sign; exiting now")
            sys.exit(0)
    weights = get_weightage(df, dep_index, indep_index, sign_vector)
    weight_copy = np.array(weights)
    if weight_copy.sum() == 0:
        continue
    line_to_write = "PWEIGHTS " + portfolio
    for weight in weights:
        line_to_write += " " + str(weight)
    line_to_write += '\n'
    out_file.write(line_to_write)
out_file.close()
