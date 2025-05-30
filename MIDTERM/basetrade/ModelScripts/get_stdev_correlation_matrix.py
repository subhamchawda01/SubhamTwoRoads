#!/usr/bin/python

"""
This is a helper script for compute_lrdb_db which takes as input the matrix datagen file and outputs the stdev and correlation matrix

"""
import pandas as pd
import numpy as np
import sys
import os

sys.path.append(os.path.expanduser('~/basetrade/'))

USAGE  = "<script> <input_file_name>"

if(len(sys.argv) < 2):
    print(USAGE + "\n")
    sys.exit(0)

data_file = sys.argv[1]

df = pd.read_csv(data_file, sep = " ", header = None)

df = df.drop(len(df.columns)-1,axis =1)


median_list = list(df.abs().median())

original_len = df.shape[0]

def filter(df_, param):
    median_values = param * df_.abs().median()

    for i in range(len(df_.columns)):
        df_ = df_[abs(df_[i]) < median_values[i]]


    return df_
median_values =  df.abs().median()


df = filter(df, 10)

line_to_write = ""
for std in df.std():
    line_to_write += str(std) + " "
line_to_write.rstrip()

print(line_to_write)


matrix_shape = df.corr().shape[0]
for i in range(matrix_shape):
    line_to_write = ""
    for j in range(matrix_shape):
        line_to_write += str(df.corr().iloc[i,j]) + " "
    print(line_to_write)



