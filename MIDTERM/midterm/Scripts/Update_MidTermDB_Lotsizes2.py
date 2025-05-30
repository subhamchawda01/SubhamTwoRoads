import pandas as pd
import sqlite3
import datetime as dt
import os, sys
import pickle

year_    = sys.argv[1]
db_path_ = sys.argv[2]


pk = pickle.load(open('/home/adey/basequant/hftrap/MediumTermStrategies/GeneralStockInfo/Historical_lotsize/NSE_lotsizes_hist_'+ year_,'rb'))
conn = sqlite3.connect(db_path_)

for date_ in pk:
    for stock_ in pk[date_]:
        #print(date_)
        #print(stock_)
        #print(pk[date_][0][stock_])
        stmt_ = "insert into LOTSIZES(day, stock, expnum ,expiry, lotsize) values(" + str(date_) + ", '" + str(stock_) + "'," + str('0') + "," + str(20050102) + ", " + str(pk[date_][stock_]) + ");"
        conn.execute(stmt_)

conn.commit()
