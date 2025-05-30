import pandas as pd
import datetime as dt
import matplotlib.pyplot as plt
import argparse

pd.set_option('display.width', 1000)

parser = argparse.ArgumentParser()
parser.add_argument("--file", help="Input the file to read the data from.")

args = parser.parse_args()
if args.file:
    file = args.file

df = pd.read_csv(file, header=None, delim_whitespace=True)
df.columns = ['time', 'sym', 'action', 'position', 'price', 'O', 'bid', 'X', 'ask', 'C']

df.time = df.time.apply(lambda x: dt.datetime.fromtimestamp(x))

df['PNL'] = 0
df.loc[df.action == 'B', 'CASH'] = -1 * df.position * df.price
df.loc[df.action == 'S', 'CASH'] = df.position * df.price

df['POS'] = 0
df.loc[df.action == 'B', 'POS'] = df.position
df.loc[df.action == 'S', 'POS'] = -1 * df.position

if(df.POS.sum() == 0):
    print("All positions closed!")
else:
    print("Position open: ", df.POS.sum())

PNL = df.CASH.sum()
NAV = 1e7 + PNL
print("NAV = ", NAV)

time_interval = df.time.iloc[-1] - df.time.iloc[0]
no_of_years = time_interval.total_seconds() / (86400 * 365)

ann_ret = round(((PNL / 1e7) / no_of_years), 4)
ann_ret *= 100


print("Ann Ret: ", ann_ret, "%")

print("Traded positions: ", df.position.sum())
print("Traded notional: ", df.CASH.abs().sum())
