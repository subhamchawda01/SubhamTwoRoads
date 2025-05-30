import pandas as pd
import sys
import os
import argparse
import getpass
import numpy as np
import datetime as dt
import matplotlib.pyplot as plt

pd.set_option('display.width', 1000)


# takes a timeseries of points PnL and extracts return / vol / sharpe etc from it
def NAVstats(NAV, init_flag=1, details=True, not_annualise=False):
    # init_flag decides whether initial value needs to be set by the function or not
    # if this is set to 1 , function calculates total drawdown and initalizes NAV at 3 x drawdown
    perf = pd.DataFrame([])
    NAV = NAV.fillna(0)

    peak_to_date = np.maximum.accumulate(NAV)

    # if init_flag==1:
    #   NAV = NAV + (3*perf.DD[0] - NAV[0])
    # # assumed that there there is no rebalancing or compounding , can change this later
    # so return is just annualised differemce in initial and final NAV
    # i.e. units of ret, stdev, DD are all points
    years = NAV.index[-1] - NAV.index[0]  # this is a timedelta object
    years = years.days / 365  # extract the days from it and div by 252
#   try:
    annualizing_factor = min(NAV.index[-1] - NAV.index[-2], NAV.index[-2] - NAV.index[-3])
    # except IndexError: #to handle the case where there are just 2 rows in NAV
    #   annualizing_factor = NAV.index[-1] - NAV.index[-2]
    # print(annualizing_factor)
    annualizing_factor = (252 * 6.5 * 3600) / (annualizing_factor.days * 6.5 * 3600 + annualizing_factor.seconds)
    if not_annualise:
        perf['Return'] = [(NAV[-1] / NAV[0]) - 1] * 1
    else:
        perf['Return'] = [(NAV[-1] / NAV[0])**(1 / years) - 1] * 1
    ret = NAV.diff(1).div(NAV)
    perf['StDev'] = [ret.std() * np.sqrt(annualizing_factor)] * 1
    perf['Sharpe'] = [perf.Return[0] / perf.StDev[0]] * 1
    perf['WorstDDEnd'] = [np.argmax(peak_to_date.div(NAV) - 1)] * 1
    perf['WorstDDStart'] = [np.argmax(NAV[:perf.WorstDDEnd[0]])] * 1
    perf['DD'] = [NAV[perf.WorstDDStart[0]] - NAV[perf.WorstDDEnd[0]]] * 1 / NAV[perf.WorstDDStart[0]]
    perf['Current_DD'] = [(NAV.iloc[-1] / peak_to_date.iloc[-1] - 1) * -1]  # print(perf['Current_DD'])
    perf['Calmar'] = [perf.Return[0] / perf.DD[0]] * 1
    perf = perf[['Return', 'StDev', 'Sharpe', 'DD', 'Current_DD', 'Calmar', 'WorstDDStart', 'WorstDDEnd']]
    last_day = NAV.index[-1]
    perf['Range'] = (NAV.max() - NAV.min()) / NAV.mean()
    perf['Ret_last_1d'] = [(NAV[-1] / NAV.loc[last_day + dt.timedelta(days=-1):].iloc[0]) - 1] * 1
    perf['Ret_last_1w'] = [(NAV[-1] / NAV.loc[last_day + dt.timedelta(days=-7):].iloc[0]) - 1] * 1
    perf['Ret_last_1m'] = [(NAV[-1] / NAV.loc[last_day + dt.timedelta(days=-30):].iloc[0]) - 1] * 1

    if details == False:
        return perf
    for sampling in ['1D', '1W', '1M']:

        series = NAV  # .resample(sampling).dropna()
        if sampling == '1D':
            timeindex = NAV.index + dt.timedelta(days=1)
        elif sampling == '1W':
            timeindex = NAV.index + dt.timedelta(days=7)
        elif sampling == '1M':
            timeindex = NAV.index + dt.timedelta(days=30)

        shiftedseries = NAV.copy()
        shiftedseries.index = timeindex
        rets = (series.div(shiftedseries) - 1).dropna()
        if rets.empty:  # happens if not enough data.
            continue
        perf['AvgRet' + sampling + '(%)'] = rets.mean() * 100
        perf['StdRet' + sampling + '(%)'] = rets.std() * 100
        perf['Worst 5% ' + sampling + '(%)'] = rets.quantile(0.05) * 100
        perf['Worst 1% ' + sampling + '(%)'] = rets.quantile(0.01) * 100
        perf['Worst_ever_' + sampling + '(%)'] = rets.min() * 100
        perf['Worst_ever_' + sampling + ' (timestamp)'] = rets.loc[rets == rets.min()].index[-1]

    # note the [ ] *1 is necessary to convert the numbers to pseudo arrays before assigning to the dataframe
    return perf


parser = argparse.ArgumentParser()
parser.add_argument("--file", help="paths of output files")
parser.add_argument("--title", help="title of the graph and pickle dump")
parser.add_argument("--statstart", help="comparison startdate in YYYYMMDD")

args = parser.parse_args()
if args.file:
    file = args.file
if args.title:
    title = args.title
if args.statstart:
    statstart = args.statstart

with open(file) as f:
    paths = f.readlines()

df_arr = []
for path in paths:
    path = path.strip()
    df = pd.read_csv(path, header=None, delim_whitespace=True)
    df.columns = ['Timestamp', 'NAV']
    df.set_index(['Timestamp'], inplace=True)
    df_arr.append(df)

cdf = pd.concat(df_arr, axis=1)

cdf = cdf.dropna()
cdf['SUM'] = cdf.sum(axis=1)

cdf = cdf[['SUM']]

cdf.index = pd.to_datetime(cdf.index, unit='s')
cdf = cdf[statstart:]

NAVperf = NAVstats(cdf.SUM, details=False)

fig = plt.figure()
ax1 = fig.add_subplot(111)
ax1.plot(cdf.index, cdf.SUM, label='NAV')
stats_str = 'Ann. Ret %0.1f %% \nAnn. Vol %0.1f %%\nDD %0.1f %% \nSharpe %0.2f \nCalmar %0.1f' % (
    NAVperf.Return * 100, NAVperf.StDev * 100, NAVperf.DD * 100, NAVperf.Sharpe, NAVperf.Calmar)
ax1.plot([NAVperf.WorstDDStart, NAVperf.WorstDDEnd], [cdf.SUM[NAVperf.WorstDDStart], cdf.SUM[NAVperf.WorstDDEnd]], 'ro')
xlims = ax1.get_xlim()
ylims = ax1.get_ylim()
ax1.annotate(stats_str, xy=(xlims[0] + 0.5 * (xlims[1] - xlims[0]), ylims[0] + 0.05 * (ylims[1] - ylims[0])))
plt.title(title)

fig.savefig(title, dpi=fig.dpi)
