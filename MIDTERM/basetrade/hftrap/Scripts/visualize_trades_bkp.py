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
nav = nav[['time', 'uts', 'numlot1', 'numlot2', 'zscore', 'PX1', 'PX2', 'Beta']]
nav['numlot1'] = -1 * nav.uts * nav.numlot1
nav['numlot2'] = nav.uts * nav.numlot2

# populate theoretical buy and sell prices for slippage analysis
nav['theoPX1'] = None
nav['theoPX2'] = None
nav.loc[nav.uts.diff(-1) != 0, 'theoPX1'] = nav.PX1.shift(-1)
nav.loc[nav.uts.diff(-1) != 0, 'theoPX2'] = nav.PX2.shift(-1)

nav['theoBuyPX'] = None
nav['theoSellPX'] = None
nav.loc[nav.uts.diff(-1) == 1,  'theoBuyPX'] = nav.theoPX1
nav.loc[nav.uts.diff(-1) == -1, 'theoBuyPX'] = nav.theoPX2
nav.loc[nav.uts.diff(-1) == 1,  'theoSellPX'] = nav.theoPX2
nav.loc[nav.uts.diff(-1) == -1, 'theoSellPX'] = nav.theoPX1

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

for col_name in names + ['uts', 'numlot1', 'numlot2', 'zscore', 'theoBuyPX', 'theoSellPX']:
    u[col_name] = u[col_name].fillna(method='pad')

# Calculate slippage
u['BuySlippage'] = 0
u['SellSlippage'] = 0

# Buy slippage is (theoreticalPX - execPX)*position
u.loc[u.action == 'B', 'BuySlippage'] = (u.theoBuyPX - u.price) * u.position
# Sell slippage is (execPX - theoreticalPX)*position
u.loc[u.action == 'S', 'SellSlippage'] = (u.price - u.theoSellPX) * u.position

u['CumBuySlip'] = u.BuySlippage.cumsum()
u['CumSellSlip'] = u.SellSlippage.cumsum()
u['CumSlippage'] = u.CumBuySlip + u.CumSellSlip

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
ax2 = fig.add_subplot(311)
plt.title('Trade Execution')
ax2.xaxis.set_major_formatter(formatter)
for name in names:
    ax2.plot(u.index, u[name], 'r')
ax2.plot(u.index, u.numlot2, 'g', linewidth=3, alpha=0.5, label="Theoretical Lots leg1")
ax2.plot(u.index, u.numlot1, 'b', linewidth=3, alpha=0.5, label="Theoretical Lots leg2")

ax3 = ax2.twinx()
ax3.plot(u.index, u.zscore, 'k', alpha=0.3, label='ZScore')
ax3.plot(u.index, u.upthres, 'brown', alpha=0.3, linewidth=1, label='ZScore threshold')
ax3.plot(u.index, u.downthres, 'brown', alpha=0.3, linewidth=1, label='ZScore threshold')
ax3.legend(loc=4)

ax4 = fig.add_subplot(312)
plt.title('Price Movements')
ax4.plot(u.index, u.PX1, label='Price_1')
ax4.plot(u.index, u.PX2 * u.Beta, label='Beta*Price_2')
ax4.xaxis.set_major_formatter(formatter)
ax4.legend(loc=1)

ax5 = ax4.twinx()
ax5.plot(u.index, u.Beta, 'k', alpha=0.5,  label='Beta')
ax5.legend(loc=4)

ax1 = fig.add_subplot(313)
plt.title('Cumulative Slippage')
ax1.plot(u.index, u.CumBuySlip, label='Buy Slippage')
ax1.plot(u.index, u.CumSellSlip, label='Sell Slippage')
ax1.plot(u.index, u.CumSlippage, label='Total Slippage')
ax1.legend()

plt.show(block=False)
