#!/usr/bin/env python

import json
from tabulate import tabulate
from shutil import copyfile
import numpy as np
import pandas as pd

data = {}
data_prev = {}

src = '/mnt/sdf/logs/json_file'
dst = '/mnt/sdf/logs/json_file_prev'

with open(src) as f:
    data = json.load(f)

with open(dst) as f:
    data_prev = json.load(f)

email_msg = []
for key, value in data.items():
    total_time = float(value[0])
    num_vals = int(value[1])
    avg_time = total_time / num_vals
    if key in data_prev:
        prev_avg_time = float(data_prev[key][0]) / int(data_prev[key][1])
        change_percent = 100 * (abs(avg_time - prev_avg_time) / prev_avg_time)
        if change_percent >= 20 or avg_time > 1800:
            email_msg.append([key, avg_time, change_percent])
    elif avg_time > 1800:
        email_msg.append([key, avg_time, 0])


copyfile(src, dst)
if len(email_msg) > 0:
    print(tabulate(email_msg, headers=["Exec", "Avg Time", "Change Percent(%)"]))
