#!/usr/bin/env python
'''
\author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
Address:
    Suite No 353, Evoma, #14, Bhattarhalli,
    Old Madras Road, Near Garden City College,
    KR Puram, Bangalore 560049, India
    +91 80 4190 3551
'''
import MySQLdb
import sys
import os
import argparse
import os
from sqlalchemy import create_engine

sys.path.append(os.path.expanduser('~/basetrade/PyScripts'))

import pandas as pd
import subprocess
import random
from datetime import datetime

import numpy as np


'''
train_test == 1 => train
train_test == 0 => test
mysql> describe dates;
+------------+-----------------+------+-----+---------+-------+
| Field      | Type            | Null | Key | Default | Extra |
+------------+-----------------+------+-----+---------+-------+
| date       | int(8) unsigned | NO   |     | NULL    |       |
| train_test | tinyint(1)      | NO   |     | NULL    |       |
+------------+-----------------+------+-----+---------+-------+
2 rows in set (0.00 sec)
'''

'''
returns n/all test dates >= min_date and/or <= max_date
'''


def read_table(query):
    engine = create_engine("mysql+mysqldb://dvcreader:f33du5rB@52.87.81.158/results")
    data = None
    try:
        data = pd.read_sql_query(query, engine)
    except MySQLdb.Error as e:
        print(query)
        print(e)
    engine.dispose()
    return(data)


def get_testdates(min_date=None, max_date=None, n=None):
    query = "SELECT date from dates WHERE train_test = " + str(0)

    if min_date is not None:
        query += " AND date >= " + str(min_date)
    if max_date is not None:
        query += " AND date <= " + str(max_date)

    query += " ORDER BY date"

    if n is not None:
        query += " desc LIMIT " + str(n)

    query += ";"
    testdates = read_table(query)
    return testdates["date"].tolist()
    #return(read_table(query))


'''
returns n/all train dates >= min_date and/or <= max_date
'''


def get_traindates(min_date=None, max_date=None, n=None):
    query = "SELECT date from dates WHERE train_test = " + str(1)

    if min_date is not None:
        query += " AND date >= " + str(min_date)
    if max_date is not None:
        query += " AND date <= " + str(max_date)

    query += " ORDER BY date"

    if n is not None:
        query += " desc LIMIT " + str(n)

    query += ";"
    traindates = read_table(query)
    return traindates["date"].tolist()
    #return(read_table(query))


'''
add all weekdays between min_date and max_date as test_date or train_date
'''


def add_dates(min_date=20100101, max_date=20201231):
    conn = MySQLdb.connect(host="52.87.81.158", user='dvcwriter', passwd="f33du5rB", db="results")
    cursor = conn.cursor()

    min_date = datetime.strptime(str(min_date), "%Y%m%d")
    max_date = datetime.strptime(str(max_date), "%Y%m%d")
    date = min_date

    df = pd.DataFrame(columns=['date', 'train_test'])
    date_list = pd.bdate_range(min_date, max_date)
    date_list = date_list.strftime('%Y%m%d')
    df['date'] = date_list
    sql_query = "INSERT INTO dates (date, train_test) VALUES(%s,%d) ON DUPLICATE KEY UPDATE train_test = %d;"
    sql_query_insert = ""
    for i in range(len(date_list)):
        date = date_list[i]
        if (is_event_day(date)):
            df.train_test[i] = random.randint(0, 1)
            sql_query_insert = sql_query % (date, df.train_test[i], df.train_test[i])

        else:
            if (int(date) % 2 == 0):
                df.train_test[i] = 0
                sql_query_insert = sql_query % (date, 0, 0)
            else:
                df.train_test[i] = 1
                sql_query_insert = sql_query % (date, 1, 1)

        cursor.execute(sql_query_insert)
        conn.commit()
        # using odd/even
        # using rand function
        # using other logic ( event flgs )

    cursor.close()
    conn.close()

def add_multiple_dates(min_date=20100101, max_date=20201231):
    engine = create_engine("mysql+mysqldb://dvcwriter:f33du5rB@52.87.81.158/results")
    min_date = datetime.strptime(str(min_date), "%Y%m%d")
    max_date = datetime.strptime(str(max_date), "%Y%m%d")
    date = min_date

    df = pd.DataFrame(columns=['date', 'train_test'])
    date_list = pd.bdate_range(min_date, max_date)
    date_list = date_list.strftime('%Y%m%d')
    df['date'] = date_list
    for i in range(len(date_list)):
        date = date_list[i]
        if (is_event_day(date)):
            df.train_test[i] = random.randint(0, 1)
        else:
            if (int(date) % 2 == 0):
                df.train_test[i] = 0
            else:
                df.train_test[i] = 1

        # using odd/even
        # using rand function
        # using other logic ( event flgs )
    df.to_sql(con=engine, name = 'dates', if_exists='append')
    engine.dispose()


def is_event_day(date):
    script = "~/basetrade_install/bin/economic_events_of_the_day"
    cmd = [script, str(date), "|grep USD | awk '{if($6>=3) print $0}' | wc -l"]
    feat = subprocess.Popen(' '.join(cmd), shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    out, err = feat.communicate()
    return(int(out))

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-m', dest='mode', help="to fill dates", type=int, required=True)
    parser.add_argument('-sd', dest='min_date', help="min date", type=int, default=None)
    parser.add_argument('-ed', dest='max_date', help="max date", type=int, required=True)
    parser.add_argument('-n', dest='num_days', help="num days", type=int, default=None)
    args = parser.parse_args()

    try:
        if args.mode == 0:
            add_multiple_dates(args.min_date, args.max_date)
        if args.mode == 1:
            add_dates(args.min_date, args.max_date)
        elif args.mode == 2:
            dates = get_traindates(args.min_date, args.max_date, args.num_days)
            print(" ".join(list(map(str,dates))))
        elif args.mode == 3:
            dates = get_testdates(args.min_date, args.max_date, args.num_days)
            print(" ".join(list(map(str,dates))))
    except Exception as e:
        print(e)
            
