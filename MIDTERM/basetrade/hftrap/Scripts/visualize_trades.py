import sys
import os
import argparse
import getpass
import itertools
import time
import calendar
import pickle
import sys
import pandas as pd
import numpy as np
import datetime as dt
import importlib as imp
import matplotlib.pyplot as plt
import scipy.stats as stats
from os import listdir
from os.path import isfile, join
from matplotlib.ticker import Formatter

pd.options.display.float_format = '{:.2f}'.format
pd.set_option('display.width', 1000)

parser = argparse.ArgumentParser()
parser.add_argument("--navpath", help="path of the nav file")
parser.add_argument("--execpath", help="path of the exec file")

args = parser.parse_args()
if args.navpath:
    navpath = args.navpath
if args.execpath:
    execpath = args.execpath

nav = pd.read_csv(navpath, sep=',\t', engine='python', header=None)
nav.columns = ['time', 'uts', 'PX1', 'PX2', 'Inst_Spread', 'Spread', 'zscore',
               'Beta', 'Intercept', 'PnlVol', 'Ready', 'C', 'numlot1', 'numlot2']
nav = nav[['time', 'uts', 'numlot1', 'numlot2', 'zscore']]
nav['numlot1'] = -1 * nav.uts * nav.numlot1
nav['numlot2'] = nav.uts * nav.numlot2
nav['longlots'] = None
nav['shortlots'] = None
nav.loc[nav.uts >= 0, 'longlots'] = nav.numlot1
nav.loc[nav.uts >= 0, 'shortlots'] = nav.numlot2
nav.loc[nav.uts <= 0, 'longlots'] = nav.numlot2
nav.loc[nav.uts <= 0, 'shortlots'] = nav.numlot1

df = pd.read_csv(execpath, header=None, delim_whitespace=True)
df.columns = ['time', 'sym', 'action', 'position', 'price', 'O', 'bid', 'X', 'ask', 'C']
df = df[['time', 'sym', 'action', 'position', 'price']]
df['pos'] = 0
df['cumpos'] = 0
df.loc[df.action == 'B', 'pos'] = df.position
df.loc[df.action == 'S', 'pos'] = -1 * df.position


df['lotsize'] = 1
# assume lotsize to be min pos for a contract
names = df['sym'].unique().tolist()

for name in names:
    df.loc[df.sym == name, 'lotsize'] = df[df.sym == name]['position'].min()
    df.loc[df.sym == name, 'cumpos'] = df[df.sym == name]['pos'].cumsum()

df['cumpos'] /= df.lotsize

df['df_index'] = df.index
dfp = df.pivot('df_index', 'sym', 'cumpos')
dfp['df_index'] = dfp.index
tu = df.merge(dfp, how='inner', on=['df_index'])

u = tu.merge(nav, how='outer', on=['time'])

u.time = u.time.apply(lambda x: dt.datetime.fromtimestamp(x))
u['old_index'] = u.index
u.sort_values(['time', 'old_index'], inplace=True)
u.reset_index(inplace=True)

for col_name in names + ['uts', 'numlot1', 'numlot2', 'longlots', 'shortlots', 'zscore']:
    u[col_name] = u[col_name].fillna(method='pad')

# horizontal line for threshold
thres = 2.5
u['upthres'] = thres
u['downthres'] = -1 * thres

# plotting functions


class MyFormatter(Formatter):
    def __init__(self, dates, fmt='%Y-%m-%d|%-H:%M:%S'):
        self.dates = dates
        self.fmt = fmt

    def __call__(self, x, pos=0):
        'Return the label for time x at position pos'
        ind = int(round(x))
        if ind >= len(self.dates) or ind < 0:
            return ''

        return self.dates[ind].strftime(self.fmt)


formatter = MyFormatter(u.time)

plt.close('all')
fig = plt.figure()
ax2 = fig.add_subplot(111)
plt.title('Path Execution')
ax2.xaxis.set_major_formatter(formatter)
for name in names:
    ax2.plot(u.index, u[name], 'r')
ax2.plot(u.index, u.numlot2, 'g', linewidth=1, alpha=0.5, label="Theoretical Lots leg1")
ax2.plot(u.index, u.numlot1, 'b', linewidth=1, alpha=0.5, label="Theoretical Lots leg2")

ax3 = ax2.twinx()
ax3.plot(u.index, u.zscore, 'k', alpha=0.3, label='ZScore')
ax3.plot(u.index, u.upthres, 'brown', alpha=0.3, linewidth=1, label='ZScore threshold')
ax3.plot(u.index, u.downthres, 'brown', alpha=0.3, linewidth=1, label='ZScore threshold')
ax3.legend(loc=4)
plt.show()
