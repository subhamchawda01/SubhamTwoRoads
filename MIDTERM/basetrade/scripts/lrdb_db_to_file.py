#!/usr/bin/env python


"""This script is intended to sync the flat files with the files already present in the db, make flat files for each date 
   ( constraint being it should be the first date of the month ) in the LRDB_base_dir, it has an option to specify another directory so that if a user wants he can 
    create the flat files without affecting the production"""

import argparse
import sys
import os
import shutil
import getpass
from datetime import datetime
import MySQLdb


def load_required_data(data, t_exchanges_session, dates_list):
    '''
    This loads the exchange to session map and date_list; 
    This only appends the date if its the first of a month
    '''
    for row in data:
        row_exchange = row[4]
        row_session = row[5] + "-" + row[6]
        date = datetime.strftime(row[9], "%Y%m%d")
        if date not in dates_list and date[6:] == '01':
            dates.append(date)
        if row_exchange not in list(t_exchanges_session.keys()):
            t_exchanges_session[row_exchange] = [row_session]
        elif row_session not in t_exchanges_session[row_exchange]:
            t_exchanges_session[row_exchange].append(row_session)


def make_the_flat_files(t_exchanges_session, t_LRDB_dir, dates_list):
    for exch in list(t_exchanges_session.keys()):
        for session in t_exchanges_session[exch]:
            files_to_remove = t_LRDB_dir + "/201*_300_timed_" + exch + "_" + session + ".txt"
            os.system("rm " + files_to_remove + "> /dev/null 2>&1")
            for date in dates_list:
                file_name = t_LRDB_dir + "/" + date + "_300_timed_" + exch + "_" + session + ".txt"
                lines_to_write = ""
                for row in data:
                    if row[4] == exch and session == row[5]+"-"+row[6] and date == datetime.strftime(row[9],"%Y%m%d"):
                        lines_to_write += (row[0] + "^" + row[1] + " " + str(row[2]) + " " + str(row[3]) + " " + row[7] + "-" + row[8] + "\n")
                if lines_to_write != "":
                    f = open(file_name, "w")
                    f.write(lines_to_write)
                    f.close()

## start main function
parser = argparse.ArgumentParser()

# specify the dafault LRDB_base_dir where we will create the flat files from the db
tradeinfo_dir = "/spare/local/tradeinfo/"
spare_dir = "/spare/local/" + getpass.getuser() + "/"
LRDB_dir = tradeinfo_dir + "/NewLRDBBaseDir/"
LRDB_dir_bkp = spare_dir + "/NewLRDBBaseDir_bkp/"
RetLRDB_dir = tradeinfo_dir + "/NewRetLRDBBaseDir/"
RetLRDB_dir_bkp = spare_dir + "/NewRetLRDBBaseDir_bkp"
LRDB_Dated_table = "LRDB_dated"
LRDB_Pair_timings_table = "LRDB_Pair_Timings"
RetLRDB_Dated_table = "Ret_LRDB_dated"
RetLRDB_Pair_timings_table = "RetLRDB_Pair_Timings"

parser.add_argument("--directory", "-d", help = "dir to output the flat files")
parser.add_argument("--is_returns", "-r", help = "Change or Returns", default = 0)
args = parser.parse_args()

is_returns = int(args.is_returns)

DIR = LRDB_dir
Dated_table = LRDB_Dated_table
Pair_timings_table = LRDB_Pair_timings_table
if is_returns == 1:
    DIR = RetLRDB_dir
    Dated_table = RetLRDB_Dated_table
    Pair_timings_table = RetLRDB_Pair_timings_table

#we only change the DIR in case it is provided by user else it is LRDB or RetLRDB
if args.directory != None:
    DIR = args.directory

#specify the backup dir
DIR_bkp = DIR.rstrip('/') + '_bkp'
if DIR == LRDB_dir:
    DIR_bkp = LRDB_dir_bkp
if DIR == RetLRDB_dir:
    DIR_bkp = RetLRDB_dir_bkp


# connect to mysql server
conn = MySQLdb.connect (host = "52.87.81.158", user = "dvcwriter", passwd = "f33du5rB", db = "lrdb")
cursor = conn.cursor()


# query to get all the beta values along with exchange, eschange_start_end_time and session_start_end_time
query = "select L.dep,L.indep, L.beta,L.correlation,S.exchange,S.start_time,S.end_time,LPT.start_time,LPT.end_time,\
        L.date_generated  from " + Dated_table + " L, session_timings S, " + Pair_timings_table + \
        " LPT where L.session_id = S.session_id and L.dep = LPT.dep and L.indep = LPT.indep and L.session_id = LPT.session_id;"

#execute the query
cursor.execute(query)

#get the data
data = cursor.fetchall()

# initiliase the map for exchange to session and a list of dates
exchanges_session = {}
dates = []

if os.path.exists(DIR):
    # remove the backup if it already exists
    if os.path.exists(DIR_bkp):
        shutil.rmtree(DIR_bkp)

    # make the backup
    shutil.copytree(DIR , DIR_bkp)
else:
    os.makedirs(DIR)

# the exchanges_session argument was added to make the function operate only on variables in the scope of the function
load_required_data(data, exchanges_session, dates) 

# the exchanges_session argument was added to make the function operate only on variables in the scope of the function
make_the_flat_files(exchanges_session, DIR, dates)
