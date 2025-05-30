import pandas as pd
import sqlite3
import datetime as dt
import os, sys

date_ = sys.argv[1]
db_path_ = sys.argv[2]

df = pd.read_csv('/spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.'+ date_, delim_whitespace=True, header = None)

df = df.loc[(df[0] == 'STKFUT') | (df[0] == 'IDXFUT') ]

print(df.head())
expiries = df.loc[df[1] == 'NIFTY'][5].values

expiries.sort()

exp_map = {expiries[0]:0,  expiries[1] : 1,  expiries[2]: 2}

df['Exp_num'] = df[5].apply(lambda x: exp_map[x])

df = df[[1,5,'Exp_num',3]]
print(expiries)
conn = sqlite3.connect(db_path_)

for key, val in df.iterrows():
    stmt_ = "insert into LOTSIZES(day, stock, expnum ,expiry, lotsize) values(" + str(date_) + ", '" + str(val[1]) + "'," + str(val['Exp_num']) + "," + str(val[5]) + ", " + str(val[3]) + ");"
    conn.execute(stmt_)

conn.commit()
