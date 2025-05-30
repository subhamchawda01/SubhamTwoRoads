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

pd.options.display.float_format = '{:.2f}'.format
pd.set_option('display.width', 1000)

parser = argparse.ArgumentParser()
parser.add_argument("--path", help="path of the file/directory to read the data from.")
parser.add_argument("--dir", help="Is the path a directory?")
parser.add_argument("--bash", help="Are you a shell script trying to run me?")

directory = False
bash = False
args = parser.parse_args()
if args.path:
    path = args.path
if args.dir:
    directory = True
if args.bash:
    bash = True

if directory:
    allfiles = [f for f in listdir(path) if isfile(join(path, f))]
else:
    allfiles = ['']

# Functions


def convertPathtoReturn(s, pos):
    # given a time series of prices and a position,
    # this generates a "return" series where it is assumed that
    # position is taken on the first available price and held to the end
    # pos should be ideally -1 or 1
    ret_series = s.copy()
    initprice = s.iloc[0]
    ret_series = ret_series / initprice - 1
    ret_series = ret_series * pos
    return ret_series


def split_TS(df, values_field, impfields=[], startflag_field=None, endflag_field=None, walkforward=20, convert_to_return=False):
    allpaths = []
    allpaths_info = {}
    if startflag_field == None:
        print('Please specify the field to split the time series by')
        return
    starts = df.loc[df[startflag_field] == True]
    end_t = None
    cols = []
    for start_t in starts.index:
        if (end_t != None):
            if (start_t < end_t):
                # this entry point already got included in the last path
                continue
        # each path should start at start_t and end at the immediately next end_t
        if endflag_field != None:
            thispath = df.loc[start_t:].iloc[1:]
            if thispath.empty:
                continue
            # print(thispath.head())
            if thispath.loc[thispath[endflag_field] == True].empty:
                end_t = thispath.index[-1]
            else:
                end_t = thispath.loc[thispath[endflag_field] == True].index[0]
            path = df.loc[start_t:end_t, values_field]
        else:
            # use walkforward
            end_t = start_t + BDay(2 * walkforward)
            path = df.loc[start_t:end_t, values_field].iloc[:walkforward]
        path.index = range(len(path))
        if convert_to_return == True:
            # it will assume that start flag field is the position.
            path = convertPathtoReturn(path, df.loc[start_t, startflag_field])
        # add a few fields at the top to contain information about the path
        path_info = {
            'start': start_t,
            'end': end_t,
            'startval': path.iloc[0],
            'endval': path.iloc[-1],
            'length': len(path),
            'duration': end_t - start_t,
            'underlyingreturn': (convert_to_return == False) * (path.iloc[-1] / path.iloc[0] - 1) + (convert_to_return == True) * (path.iloc[-1]),
        }
        for impfield in impfields:
            path_info['start_' + impfield] = df.loc[start_t, impfield]
            path_info['end_' + impfield] = df.loc[end_t, impfield]
        cols.append(start_t)
        allpaths.append(path)
        allpaths_info[start_t] = (path_info)

    if allpaths != []:
        allpaths = pd.concat(allpaths, axis=1).T
        allpaths.index = cols
        return [allpaths, allpaths_info]
    else:
        return [pd.DataFrame([]), pd.DataFrame([])]


def GenPathStatistics(split_paths, split_paths_info, full_path, timeperiods=[]):
    # split_path and split_path_info are the two data frames returned by split_paths
    BPS_VALUE = 10000
    # Seggregatesplit_paths into long and short
    longpaths = split_paths.T
    longpaths = longpaths.copy()

    print("Calculating mean path...")
    # initialise Res data frame
    Res = pd.DataFrame([])
    Res['long_mean'] = longpaths.mean(axis=1)
    Res['long_median'] = longpaths.median(axis=1)
    Res['#long'] = len(longpaths.columns)

    #print( "Calculating %UP paths..." )
    a = full_path
    # Calculate base statistics
    Res['base_mean'] = 0
    Res['base_median'] = 0
    # #Res['base_std']		= [np.std( a.div( a.shift(t) ).dropna() - 1 ) for t in Res.T.columns]

    #print( "Calculating p stats..." )
    # Calculate advantage of the strategy
    Res['Adv_long_mean'] = (Res['long_mean'] - Res['base_mean']) * BPS_VALUE
    Res['Adv_long_median'] = (Res['long_median'] - Res['base_median']) * BPS_VALUE
    # Res['t_val_long']			= np.sqrt(Res['#long'])*( Res['long_mean'] - Res['base_mean'] )/Res['base_std']
    # Res['p_val_long']			= stats.t.sf(Res['t_val_long'], Res['#long'])

    # Res.index = range(1,len(Res)+1)
    Res.index.name = 'bars'

    # generate separate tables for long and short.
    n_longpaths = len(longpaths.T)
    longmsg = 'Buy Signal : %d of %d Bars [%0.1f%%]' % (n_longpaths, len(full_path), 100 * n_longpaths / len(full_path))

    print("Calculating average future return...")
    tmp = split_paths.copy()
    for i in tmp.index:
        tmp.loc[i, :] = paths_info[i]['endval'] - tmp.loc[i, :]

    tmp2 = longpaths.copy()
    tmp2['fr'] = 0
    tmp2['mfr'] = 0

    for i in tmp2.index:
        tmp2.loc[i, 'fr'] = tmp[i].mean()
        tmp2.loc[i, 'mfr'] = tmp[i].median()
        tmp2.loc[i, 'up'] = len([x for x in tmp[i] if x > 0])

    tmp2['Active'] = [len([i for i in longpaths.loc[t] if not pd.isnull(i)]) for t in Res.T.columns]

    Res['fr'] = tmp2['fr'] * BPS_VALUE
    Res['mfr'] = tmp2['mfr'] * BPS_VALUE

    # %winning trade
    #Res['UP_long']		= [len([x for x in tmp.loc[t,:] if x > 0]) for t in Res.T.columns]

    Res['Active'] = 100 * tmp2['Active'] / Res['#long']
    Res['up'] = 100 * tmp2['up'] / tmp2['Active']
    #Res_long = Res[['Adv_long_mean' , 'Adv_long_median' , 'p_val_long' , 'UP_long']]
    Res_long = Res[['Adv_long_mean', 'Adv_long_median', 'fr', 'mfr', 'Active', 'up']]
    #Res_long.columns = ['Mean α', 'Median α' , 'p' , 'Up%']
    Res_long.columns = ['Mean α', 'Median α', 'mean fut ret', 'median fut ret', '% Active paths', '% win paths']
    # Res_long['Down%'] = 1-Res_long['Up%']
    # Res_long[['Up%','Down%']] = Res_long[['Up%','Down%']].applymap(lambda x : '%d%%'%(100 *x))

    # insert blank cols
    nblanks = 1
    blankcols = Res.iloc[:, :nblanks]
    blankcols.loc[:] = ""
    blankcols.columns = [''] * nblanks

    printable = {}
    if timeperiods == []:
        printable[longmsg] = Res_long
    else:
        printable[longmsg] = Res_long.loc[timeperiods]
    printable = pd.Panel(printable).to_frame().unstack()
    printable['|'] = '|'
    printable = printable[[longmsg]]

    print(printable.to_string(col_space=20))
    # printable = pd.concat([Res_long , blankcols , Res_short] , axis = 1)
    # print(printable.to_string())
    return [Res, longpaths]


wf = 8000
timeperiods = [1, 2, 3, 4, 5, 10, 15, 20, 25, 50, 75, 100, 125, 250, 375,
               500, 625, 750, 875, 1000, 1250, 1500, 2000, 3000, 4000, 5000, 6000, 8000]
timeperiods = [x for x in timeperiods if x <= wf]

# Loop over files
if bash:
    u = pd.DataFrame(columns=['position', 'zscore', 'C'])
else:
    u = pd.DataFrame(columns=['Timestamp', 'position', 'PX1', 'PX2', 'Inst_Spread',
                              'Spread', 'zscore', 'Beta', 'Intercept', 'PnlVol', 'Ready', 'C'])

for filename in allfiles:
    print("Reading ", path + filename, "...")
    # Read in a df
    if bash:
        df = pd.read_csv(path + filename, sep='\t', engine='python', header=None)
        df.columns = ['position', 'zscore', 'C']
    else:
        df = pd.read_csv(path + filename, sep=',\t', engine='python', header=None, skiprows=1)
        df.columns = ['Timestamp', 'position', 'PX1', 'PX2', 'Inst_Spread',
                      'Spread', 'zscore', 'Beta', 'Intercept', 'PnlVol', 'Ready', 'C']
    # Append dfs with "C"
    u = u.append(df, ignore_index=True)

u.dropna(inplace=True)

#Split in paths
print("Splitting paths...")
u['start_flag'] = 0
u['end_flag'] = 0
u.loc[(u.position.diff() != 0) & (u.position != 0), 'start_flag'] = 1
u.loc[(u.position.shift(-1).diff() != 0) & (u.position.shift(-1) == 0), 'end_flag'] = 1
[paths, paths_info] = split_TS(u, 'C', impfields=['position'], startflag_field='start_flag',
                               endflag_field='end_flag', convert_to_return=True, walkforward=wf)
[res, longpaths] = GenPathStatistics(paths, paths_info, u.C, timeperiods=timeperiods)
r = res[res.Active > 1]

# z-score stoploss analysis
entry_zscore_thres = 2.5
print('\n\nPerforming Z-score stoploss analysis...')
print('Total no of paths = ', len(paths_info.keys()))
print('Average return of paths after Exit_ZScore is hit for the first time:')
print("Exit_ZScore\t\tAv_Return\t\tNum_paths")

for exit_zscore in [3.5, 4, 4.5, 5, 5.5, 6, 6.5]:
    u['start_flag'] = 0
    u['end_flag'] = 0
    u.loc[((u.zscore > exit_zscore) | (u.zscore < -1 * exit_zscore)) & (u.position != 0), 'start_flag'] = 1
    u.loc[(u.position.shift(-1).diff() != 0) & (u.position.shift(-1) == 0), 'end_flag'] = 1
    [paths, paths_info] = split_TS(u, 'C', impfields=['position'], startflag_field='start_flag',
                                   endflag_field='end_flag', convert_to_return=True, walkforward=wf)

    # mean return of path
    ret_series = [paths_info[i]['endval'] for i in paths_info.keys()]
    if (len(ret_series) > 0):
        av_ret = round(sum(ret_series) / len(ret_series), 6)
    print('%10.2f\t\t%9.2f\t\t%9.2f' % (exit_zscore, 10000 * av_ret, len(ret_series)))

print("\n\nPlotting...")
plt.close('all')
fig = plt.figure()
# ax1 = fig.add_subplot(211)
# plt.title('Indiv Trade Paths')
# for t in paths.index:
# 	ax1.plot(paths.loc[t].index,paths.loc[t],alpha = 0.3)
# plt.grid()

ax2 = fig.add_subplot(111)
plt.title('Portfolio Mean')
#ax2.plot(r.index , r.long_mean , 'b', linewidth =2, label = "Mean")
#ax2.plot(r.index , r.long_median , 'g', linewidth =1, label = "Median" )
ax2.plot(r.index, r.fr, linewidth=2, label="Mean Fut Ret")
ax2.plot(r.index, r.mfr, linewidth=1, label="Median Fut Ret")
ax2.legend(loc=2)

ax3 = ax2.twinx()
ax3.plot(r.index, r.Active, 'y', label="% Active Paths")
ax3.plot(r.index, r.up, 'r', label="% win paths")
ax3.legend(loc=1)
plt.grid()
plt.show(block=True)
