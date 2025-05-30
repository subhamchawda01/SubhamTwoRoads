import sys, os
import sqlite3
import pandas as pd
import datetime as dt

stock_ = sys.argv[1]
df_ = pd.read_csv('/data1/apps/data/NSEBarData/Archive/Fut_Temp/' + stock_, delimiter='\t', header=None)
df_ = df_.loc[df_[1] == stock_ + '_FF_0_0']
df_['Date'] = df_[0].apply(lambda x : int(dt.datetime.fromtimestamp(x).strftime('%Y%m%d')))

df_['Min_Close'] = df_[6]
df_['Max_Close'] = df_[6]

df1_ = df_.groupby('Date').agg({'Min_Close':min, 'Max_Close':max})

conn = sqlite3.connect('/home/dvctrader/trash/midterm_db_temp')

for key, val in df1_.iterrows():
    try:
        conn.execute("insert into MIN_MAX_PRICES(day, stock ,min ,max ) values(" + str(key) + ",'" + stock_ +"'," + str(val['Min_Close']) + "," + str(val['Max_Close']) + ");")
    except:
        conn.execute("update MIN_MAX_PRICES set min = " + str(val['Min_Close']) + " and max  = " + str(val['Max_Close']) + " where day = " + str(key) + " and stock = '" + stock_ + "';"  )

conn.commit()

