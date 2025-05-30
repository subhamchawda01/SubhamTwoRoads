import sys
import os
import argparse
import getpass
import pandas as pd
import datetime as dt

pd.set_option('display.width', 800)

parser = argparse.ArgumentParser()
parser.add_argument("--margin", help="margin for each pair")
parser.add_argument("--file", help="path of file containing pair info")

args = parser.parse_args()
if args.margin:
    margin = int(args.margin)
if args.file:
    path = args.file

df = pd.read_csv(path, header=None, delim_whitespace=True)
df.columns = ['name1', 'name2', 'lotsize1', 'lotsize2', 'px1', 'px2']

df['ideal_Beta'] = df.px2 / df.px1

df['numlot1_100'] = (margin * 2.5) / (df.lotsize1 * df.px1)
df['numlot1_100'] = df['numlot1_100'].round()
df['numlot2_100'] = (margin * 2.5) / (df.lotsize2 * df.px2)
df['numlot2_100'] = df['numlot2_100'].round()

df['numlot1_90'] = (margin * 2.5) / (df.lotsize1 * df.px1 * 0.9)
df['numlot1_90'] = df['numlot1_90'].round()
df['numlot2_90'] = (margin * 2.5) / (df.lotsize2 * df.px2 * 0.9)
df['numlot2_90'] = df['numlot2_90'].round()

df['numlot1_110'] = (margin * 2.5) / (df.lotsize1 * df.px1 * 1.1)
df['numlot1_110'] = df['numlot1_110'].round()
df['numlot2_110'] = (margin * 2.5) / (df.lotsize2 * df.px2 * 1.1)
df['numlot2_110'] = df['numlot2_110'].round()

df['Beta_90_90'] = (df.numlot1_90 * df.lotsize1) / (df.numlot2_90 * df.lotsize2)
df['Beta_90_100'] = (df.numlot1_90 * df.lotsize1) / (df.numlot2_100 * df.lotsize2)
df['Beta_90_110'] = (df.numlot1_90 * df.lotsize1) / (df.numlot2_110 * df.lotsize2)
df['Beta_100_90'] = (df.numlot1_100 * df.lotsize1) / (df.numlot2_90 * df.lotsize2)
df['Beta_100_100'] = (df.numlot1_100 * df.lotsize1) / (df.numlot2_100 * df.lotsize2)
df['Beta_100_110'] = (df.numlot1_100 * df.lotsize1) / (df.numlot2_110 * df.lotsize2)
df['Beta_110_90'] = (df.numlot1_110 * df.lotsize1) / (df.numlot2_90 * df.lotsize2)
df['Beta_110_100'] = (df.numlot1_110 * df.lotsize1) / (df.numlot2_100 * df.lotsize2)
df['Beta_110_110'] = (df.numlot1_110 * df.lotsize1) / (df.numlot2_110 * df.lotsize2)

df['max_hdg_imprf'] = 0

for i in df.index:
    arr = [abs(x - df['ideal_Beta'][i]) / df['ideal_Beta'][i] for x in [df['Beta_90_90'][i], df['Beta_90_100'][i], df['Beta_90_110'][i],
                                                                        df['Beta_100_90'][i], df['Beta_100_100'][i], df['Beta_100_110'][i], df['Beta_110_90'][i], df['Beta_110_100'][i], df['Beta_110_110'][i]]]
    df.loc[i, 'max_hdg_imprf'] = max(arr) * 100


dfp = df[['name1', 'name2', 'lotsize1', 'lotsize2', 'px1', 'px2', 'ideal_Beta', 'numlot1_100',
          'numlot2_100', 'Beta_90_110', 'Beta_100_100', 'Beta_110_90', 'max_hdg_imprf']]

print(dfp)
