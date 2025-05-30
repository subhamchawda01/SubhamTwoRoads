import matplotlib
matplotlib.use('Agg')
import sys
import os
import argparse
import getpass
import itertools
import time
import calendar
import pickle
import pandas as pd
import numpy as np
import datetime as dt
import importlib as imp
import matplotlib.pyplot as plt
from pandas.tseries.offsets import BDay

pd.set_option('display.width', 1000)

parser = argparse.ArgumentParser()
parser.add_argument("--so", help="Startoffset: Integer number of days from event, for T-5d enter -5")
parser.add_argument("--eo", help="Endoffset: Integer number of days from event, for T+5d enter +5")

args = parser.parse_args()
if args.so:
    startoffset = int(args.so)
if args.eo:
    endoffset = int(args.eo)

ed = pd.read_pickle("earningsdates_v2.pkl")
sm = pd.read_csv("sectormapping.csv", header=None)

files = os.listdir('/spare1/ssingh/test/MT_SPRD_NAV_SERIES/')
Yfiles = [f for f in files if(f.find('_12_') != -1)]

res = pd.DataFrame(columns=['symbol', 'date', 'filepath'])
for name in ed.index:
    dates = [dt.datetime.strptime(d1, '%Y-%m-%d').date() for d1 in ed.loc[name] if str(d1) != 'nan']
    for date in dates:
        for f in [file for file in Yfiles if ((file.find(name) != -1) and (os.stat('/spare1/ssingh/test/MT_SPRD_NAV_SERIES/' + file).st_size > 0))]:
            sd = dt.datetime.strptime(f.split('_')[4], '%Y%m%d').date()
            if((f.find(name) != -1) and ((date - sd).days > -1.4 * startoffset) and ((date - sd).days < (365 - 1.4 * endoffset))):
                res.loc[len(res)] = [name, date, f]
                break

print('Generating ' + str(len(res)) + ' paths...')
df_arr = []
for i in res.index:
    row = res.iloc[i]
    print(row.symbol, row.date, row.filepath)
    startdate = row.date + np.sign(startoffset) * BDay(abs(startoffset))
    enddate = row.date + np.sign(endoffset) * BDay(abs(endoffset) + 1)

    command = 'cat /spare1/ssingh/test/MT_SPRD_NAV_SERIES/' + row.filepath + """ |awk -F ',\\t' -v "ST=""" + \
        startdate.strftime('%s') + """" -v "ET=""" + enddate.strftime('%s') + \
        """" '{if (($1 > ST)&&($1 < ET)) print $12}'>tmp """
    os.system(command)
    if (os.stat('tmp').st_size == 0):
        print('Missing data in ' + row.filepath)
        continue
    df = pd.read_csv('tmp', header=None)
    df.columns = ['res' + str(i)]
    df = df.div(df.loc[0])
    if max(abs(df['res' + str(i)].diff(1)).dropna()) > 0.1:
        print('Unexpected jump in ' + row.filepath)
    else:
        df_arr.append(df)

paths = pd.concat(df_arr, axis=1)
paths = paths.fillna(method='pad')
paths = paths.div(paths.loc[0])
paths = paths - 1

result = pd.DataFrame([])
result['Mean'] = paths.mean(axis=1)
result['StdDev'] = paths.std(axis=1)
result['meanByStd'] = result['Mean'] / result['StdDev']

allpath = paths.T
allpath.reset_index(inplace=True, drop=True)

wf = (endoffset - startoffset) * 375
print(result.loc[np.arange(0, wf, 200)])

plt.close('all')
fig = plt.figure()
ax1 = fig.add_subplot(211)
plt.title('Paths: [' + str(startoffset) + ',' + str(endoffset) + ']')
for i in allpath.index:
    ax1.plot(allpath.iloc[i].index, allpath.iloc[i], alpha=0.3)
ax1.set_xlim([0, wf])
plt.grid()

ax2 = fig.add_subplot(212)
plt.title('Portfolio Mean')
ax2.plot(result.index, result.meanByStd, 'k', label="meanByStd")
ax2.plot(result.index, result['StdDev'], 'r', label="STD")
ax2.set_xlim([0, wf])

ax3 = ax2.twinx()
ax3.plot(result.index, result['Mean'], 'b', linewidth=2, label="Mean")
plt.grid()

fig.savefig('EarningImpactAnalysis_' + str(startoffset) + '_' + str(endoffset), dpi=fig.dpi)
