import sys
import os
import argparse
import getpass
import pandas as pd
import datetime as dt

pd.set_option('display.width', 1000)

parser = argparse.ArgumentParser()
parser.add_argument("--sym", help="symbol of the underlying")
parser.add_argument("--start", help="startdate in ddmmyyyy")
parser.add_argument("--end", help="enddate in ddmmyyyy")

args = parser.parse_args()
if args.sym:
    sym = args.sym
if args.start:
    start = args.start
if args.end:
    end = args.end

path = '/NAS1/data/NSEBarData/FUT_BarData/'

df = pd.read_csv(path + sym, header=None, delim_whitespace=True)
df.columns = ['time', 'sym', 'st', 'et', 'exp', 'O', 'C', 'L', 'H', 'V', 'N']

# filter out only front months
df = df[df.sym.str.contains('_0_0')]

if len(df) == 0:
    print("No symbol with suffix _0_0!")

# Add date
df.time = df.time.apply(lambda x: dt.datetime.fromtimestamp(x))
df.set_index('time', inplace=True)

df = df[start: end]

print(sym, "\t", df.V.sum())
