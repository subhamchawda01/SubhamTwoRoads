import pandas as pd
import pickle
import datetime as dt
import os, sys

date_ = sys.argv[1]

df = pd.read_csv('/spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.'+ date_, delim_whitespace=True, header = None)
pk = pickle.load(open('/home/adey/midterm/Scripts/abc','rb'))

df = df.loc[(df[0] == 'STKFUT') | (df[0] == 'IDXFUT') ]
expiries = df.loc[df[1] == 'NIFTY'][5].values
expiries.sort()

df = df[[1,5,3]]
df = df.loc[df[5] == expiries[0]]
pk[str(date_)] = {}
for key, val in df.iterrows():
    pk[str(date_)][val[1]] = int(val[3])
pickle.dump( pk, open( '/home/adey/midterm/Scripts/abc', 'wb' ))
