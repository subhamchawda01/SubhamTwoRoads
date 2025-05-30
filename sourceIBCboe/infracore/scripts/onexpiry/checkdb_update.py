import pandas as pd
import sqlite3
import sys
import os.path

if ((len(sys.argv) < 3 or len(sys.argv) > 3)):
    print ("Usage:  theScript date(YYYYMMDD) midtermdb_path")
    sys.exit(0)

date_ = sys.argv[1]
db_path_ = sys.argv[2]

if not  os.path.exists('/spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_'+ date_+'.csv'):
    print("Lot size file not exist in db check")
    exit(0)

df = pd.read_csv('/spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_'+ date_+'.csv', header = [1])
try:
    conn = sqlite3.connect(db_path_)
except Error as e:
    print (e)

# select lotsize from LOTSIZES where expnum=1 and day=20200201 and stock='RBLBANK';
# check for FUT0
for key, val in df.iterrows():
    stmt_ = "select lotsize from LOTSIZES where expnum=0 and day=" +str(date_) + " and stock='" + str(val[1]).rstrip() + "';"
    cursor = conn.execute(stmt_)
    row_count=0
    if str(val[1]).rstrip() == "NIFTYIT":
        continue
    for row in cursor:
        row_count+=1
        if int(row[0]) != int(val[2]):
            print ( "Diff in FUT0 lotsize for Security=" + str(val[1]).rstrip() )
    if row_count == 0:
        print("Entry does not exist for the Security=" + str(val[1]).rstrip())


# check for FUT1
for key, val in df.iterrows():
    stmt_ = "select lotsize from LOTSIZES where expnum=1 and day=" +str(date_) + " and stock='" + str(val[1]).rstrip() + "';"
    cursor = conn.execute(stmt_)
    row_count=0
    if str(val[1]).rstrip() == "NIFTYIT":
        continue
    for row in cursor:
        row_count+=1
        if int(row[0]) != int(val[3]):
            print ( "Diff in FUT1 lotsize for Security=" + str(val[1]).rstrip() )
    if row_count == 0:
        print("Entry does not exist for the Security=" + str(val[1]).rstrip())
