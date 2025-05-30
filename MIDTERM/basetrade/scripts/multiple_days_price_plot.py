#!/media/shared/ephemeral14/anaconda2/bin/python
# coding: utf-8


import cufflinks as cf
import pandas as pd
import numpy as np
import subprocess
from functools import partial

print cf.__version__

lookback = 400
import numpy as np
import os
import dateparser
import matplotlib.pyplot as plt
from datetime import datetime, timedelta

cf.set_config_file(world_readable=True, offline=False)

import plotly
import sys

print plotly.__version__
import datetime
import seaborn as sns

iris = sns.load_dataset("iris")

print sys.argv[0]

# Pass Scripts arguments
if len(sys.argv) < 10:
    print("USAGE : <script> <shortcode> SHCLIST <shortcode list> -1"
           " PORTLIST <portfolio list> -1 <start_date> <num_days> <work_dir>")
    print("EXAMPLE : ~/multiple_days_price_plot.py FGBM_0 SHCLIST FGBM_0 FGBL_0 ZN_0 -1 "
          "PORTLIST EBFUT2S UELLBOND -1 20170101 20 /home/devendra/")
    sys.exit(1)

shc = sys.argv[1]
shcList = []
count = 3
# Creating ShortCodeList
if sys.argv[2] == "SHCLIST":
    while (1):
        if sys.argv[count] == "-1" and sys.argv[count + 1] == "PORTLIST":
            count = count + 1
            break;
        else:
            shcList.append(sys.argv[count])
        count = count + 1
        if count > 50:
            print >> sys.stderr, "Either PORTLIST argument not given or large number of shortcodes given"
            sys.exit(1)

portList = []

# Creating PortfolioList
if sys.argv[count] == "PORTLIST":
    count = count + 1
    while (1):
        if sys.argv[count] == "-1":
            count = count + 1
            break
        else:
            portList.append(sys.argv[count])
        count = count + 1
        if count > 50:
            print >> sys.stderr, "Either large number of portfolios given"
            sys.exit(1)

start_date = int(sys.argv[count])
count = count + 1
num_days = int(sys.argv[count])
count = count + 1
work_dir = sys.argv[count]
ilist = work_dir + "ilist"


# Calculating Next Trading Day Function
def calc_next_day(date):
    prev_day_calc_cmd = "~/basetrade_install/bin/calc_next_week_day" + " " + str(date) + " >" + work_dir + "date_prev"
    os.system(prev_day_calc_cmd)
    s = ""
    f = open(work_dir + "date_prev", 'r')
    s = f.read()
    date_next = int(s)
    return date_next


# Create ilist for the shortcodes and portfolios whose plot is going to be plotted
f = open(ilist, 'w')
f.write("MODELINIT DEPBASE " + shc + " MktSizeWPrice MktSizeWPrice\n")
f.write("MODELMATH LINEAR CHANGE\n")
f.write("INDICATORSTART\n")
for shortcode in shcList:
    f.write("INDICATOR 1 L1Price " + shortcode + " MktSizeWPrice\n")
for portfolio in portList:
    f.write("INDICATOR 1 L1PortPrice " + portfolio + " MktSizeWPrice\n")
f.write("INDICATOREND")
f.close()

# Fixing Plot parameters
fig, ax = plt.subplots()
fig_size = [20, 10]
plt.rcParams["figure.figsize"] = fig_size
plt.rcParams["xtick.labelsize"] = 25
plt.rcParams["ytick.labelsize"] = 25
legend = ax.legend(loc='lower left')

# Generating Data in time(hrs) vs returns for given set of dates
ilist_csv = pd.read_csv(ilist, delim_whitespace=True, header=None)
date = start_date
data = pd.DataFrame()
for dd in range(0, num_days):
    cmd = "/home/dvctrader/cvquant_install/basetrade/bin/datagen " + ilist + " " + str(
        date) + " UTC_001 UTC_2359 1024 " + work_dir + "out 1000 e1 ts1 0"
    os.system(cmd)
    print cmd
    if dd == 0:
        if os.stat(work_dir + "out").st_size != 0:
            data = pd.read_csv(work_dir + "out", delim_whitespace=True, header=None)
    if dd != 0:
        if os.stat(work_dir + "out").st_size != 0:
            temp_data = pd.read_csv(work_dir + "out", delim_whitespace=True, header=None)
            msecs_in_day = 86400000 * dd
            temp_data[0] += msecs_in_day
            data = data.append(temp_data.copy()).copy()
    date = calc_next_day(date)
    print date


# Ploting the plot for each shortcode , portfolio
for index in range(4, ilist_csv.shape[0]):
    shc_data = data.iloc[:, [0, index]].copy().reindex()

    msecs_to_hrs = 3600000.0
    first_value = shc_data.iloc[0][index]
    shc_data = shc_data[shc_data[index] * 100 / first_value > 80]
    shc_data = shc_data[shc_data[index] * 100 / first_value < 120]
    print first_value
    print len(shc_data)
    print "Max", 100 * np.max(shc_data[index]) / first_value, "Min", 100 * np.min(shc_data[index]) / first_value
    plt.plot(shc_data[0] / msecs_to_hrs, shc_data[index] * 100 / first_value, label=ilist_csv[3][index - 1])

plt.legend(loc='best')
plt.show()
