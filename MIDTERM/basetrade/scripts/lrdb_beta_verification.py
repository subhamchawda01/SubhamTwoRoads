#!/usr/bin/env python

"""
This script is meant to help the trader see the deps, indeps for which dirty bit is 1, also see the previous three beta values
and lets them update those values to the previous stable value or set the dirty_bit to 0
"""

# import all the required directories
from __future__ import print_function
import sys
import argparse
import numpy as np
from datetime import datetime
import os


import MySQLdb

sys.path.append(os.path.expanduser('~/basetrade/'))
from walkforward.wf_db_utils.db_handles import connection

# This script has been written very poorly.Functions should we written
# such that they take all the arguments needed inside them.
# It takes very little effort to make functions into complete modules.
# This on the other hand has been written such that anything
# can affect anything else.
# TODO: lots

# add the required arguments to parser
parser = argparse.ArgumentParser()
parser.add_argument("--date", "-d", help="date to check")
parser.add_argument("--start_date", "-sd", help="date to check")
parser.add_argument("--end_date", "-ed", help="date to check")
parser.add_argument("--product", "-p", help="product/dep to check")
parser.add_argument("--indep", "-I", help="Indep you want to check/approve; to give a list seperate by ,", default="ALL")
parser.add_argument("--returns", "-r", help="To Use ReturnsDB", default=0)

args = parser.parse_args()

# check if length is less than 2 print help and exit
if len(sys.argv) < 2:
    parser.print_help()
    sys.exit(0)

dep = args.product
indep = args.indep
date = args.date
start_date = args.start_date
end_date = args.end_date
returns = int(args.returns)

if date != None and (start_date != None or end_date != None):
    print("Ambiguous input \n Please use either date or both start_date and end_date\n")
    parser.print_help()
    sys.exit(0)


Dated_Table = "LRDB_dated"

if returns:
    Dated_Table = "Ret_LRDB_dated"

# it just indents the index so that lines are printed uniformly


def indent_row_no(index):
    if index < 10:
        return "{0:>3}".format(index)
    elif index < 100:
        return "{0:>3}".format(index)
    else:
        return str(index)


""" this is the function that asks the user the indexes he would like to modify 
and then if he would like to set dirty bit to 0 or update the beta and correlation value to the previously existing stable value"""


def take_arguments():
    try:
        to_update = input("Enter the row numbers you would want to modify : ALL/Row_nos,None :  ")
    except KeyboardInterrupt:  # handle ctrl C
        print(" exiting...")
        sys.exit()
    if to_update == "None" or to_update == "":
        print("exiting...")
        sys.exit()
    if to_update != "ALL":  # if the entered value is not ALL it checks if the value can be converted to integer, else asks the user to input again
        # feel free to suggest/add more checks for robustness
        try:
            test = to_update.split(",")
            int(test[0])
        except:
            print("you entered an invalid input, try again : ")
            take_arguments()
    try:
        db_or_prev = int(
            input("Would you like to update dirty_bit to 0 ( enter 0) or set it to the previous stable value (enter 1) : "))
    except KeyboardInterrupt:
        print(" exiting...")
        sys.exit()

    # if the input isn't 0 or 1, it exits
    if db_or_prev != 0 and db_or_prev != 1:
        print("Expected 0 or 1")
        sys.exit()
    return to_update, db_or_prev


def update_db_to_prev(data, session_id_start_end_time, row_printed_to_index, to_update):
    if to_update == "ALL":
        to_update = all_red  # gives to_update the list of all the dirty bit 1 row indexes in the fetched 'data'
    else:
        to_update = to_update.split(",")  # convert the row indexes to a list
        to_update = [row_printed_to_index[int(element)] for element in to_update]  # convert each element to int
    for element in to_update:
        start_time_ = data[element][9]
        end_time_ = data[element][10]
        indep_to_check = data[element][1]
        date_to_check = data[element][3]
        # get the session id for the
        for keys in list(session_id_start_end_time.keys()):
            if session_id_start_end_time[keys] == start_time_ + "_" + end_time_:
                session_id = keys
            # query to update the beta and correlation to the previously existing stable pair;
        query = "update " + Dated_Table + " set beta = ( select beta from (select beta from " + Dated_Table + " where dep = '" +\
                dep + "' and indep = '" + indep_to_check + "' and date_generated < " + datetime.strftime(date_to_check, "%Y%m%d") +\
                " and dirty_bit = 0  and session_id = " + str(session_id) + " order by date_generated desc limit 1) AS x) ,correlation = ( select correlation from (select correlation from " + Dated_Table + " where dep = '" + dep + \
            "' and indep = '" + indep_to_check + "' and date_generated < " + datetime.strftime(date_to_check, "%Y%m%d") + " and dirty_bit = 0  and session_id = " + str(
                session_id) + " order by date_generated desc limit 1) AS y) , dirty_bit = 0 where dep = '" + dep + "' and indep = '" + indep_to_check + "' and session_id = " + str(session_id) + " and date_generated = " + datetime.strftime(date_to_check, "%Y%m%d") + " and dirty_bit =1;"
        cursor.execute(query)
    conn.commit()


def update_db_to_zero(data, date, start_date, end_date, session_id_start_end_time, row_printed_to_index, to_update):
    if to_update == "ALL" and date != None:
        query_to_update = "update " + Dated_Table + " set dirty_bit = 0 where date_generated = " + \
            date + " and dirty_bit = 1 and dep = '" + dep + "';"
        cursor.execute(query_to_update)
    elif to_update == "ALL" and start_date != None and end_date != None:
        query_to_update = "update " + Dated_Table + " set dirty_bit = 0 where date_generated <= " + end_date + \
            " and date_generated >= " + start_date + " and dirty_bit = 1 and dep = '" + dep + "';"
        cursor.execute(query_to_update)
    else:
        to_update = to_update.split(",")
        try:
            int(to_update[0])
            for element in to_update:
                if element not in row_printed_to_index:
                    print("you entered an invalid input, try again : ")
                    take_arguments()
        except:
            print("you entered an invalid input, try again : ")
            take_arguments()
            # sys.exit()
        for element in to_update:
            element = (row_printed_to_index[int(element)])
            i = (element)
            start_time_ = data[i][9]
            end_time_ = data[i][10]
            for keys in list(session_id_start_end_time.keys()):
                if session_id_start_end_time[keys] == start_time_ + "_" + end_time_:
                    session_id = keys
            query_to_update = "update " + Dated_Table + " set dirty_bit = 0 where date_generated = " + datetime.strftime(data[int(
                element)][3], "%Y%m%d") + " and dep = '" + dep + "' and indep = '" + data[int(element)][1] + "' and session_id = " + str(session_id) + ";"
            cursor.execute(query_to_update)
    conn.commit()

# this function queries the lrdb db for the given start_date,end_date and returns the fetched data


def get_data_start_date_end_date(start_date, end_date, active_db_cursor):

    # if indep is all queries for dep with no condition on indep else queries for indep in (given indeps) else for indep in the given arguments
    if indep == "ALL":
        query = "select Ld.*,S.exchange, S.start_time, S.end_time  from " + Dated_Table + " Ld, session_timings S where date_generated <= " + \
            end_date + " and dep = '" + dep + "'"
    else:
        indep_list = indep.split(",")
        query = "select Ld.*,S.exchange, S.start_time, S.end_time  from " + Dated_Table + " Ld, session_timings S  where date_generated <= " + \
            end_date + " and dep = '" + dep + "' and indep in ("
        for i in range(len(indep_list) - 1):
            query += "'" + indep_list[i] + "',"
        query += "'" + indep_list[-1] + "')"
    query += " and Ld.session_id = S.session_id order by dep, indep,start_time,end_time, date_generated asc"
    active_db_cursor.execute(query)
    data = active_db_cursor.fetchall()
    return data


def get_data_date(date, active_db_cursor):
    if indep == "ALL":
        query = "select Ld.*,S.exchange, S.start_time, S.end_time  from " + Dated_Table + " Ld, session_timings S where date_generated <= " + \
            date + " and dep = '" + dep + "'"
    else:
        indep_list = indep.split(",")
        query = "select Ld.*,S.exchange, S.start_time, S.end_time  from " + Dated_Table + " Ld, session_timings S  where date_generated <= " + \
            date + " and dep = '" + dep + "' and indep in ("
        for i in range(len(indep_list) - 1):
            query += "'" + indep_list[i] + "',"
        query += "'" + indep_list[-1] + "')"
    query += " and Ld.session_id = S.session_id order by dep, indep, start_time,end_time,date_generated desc"
    active_db_cursor.execute(query)
    data = active_db_cursor.fetchall()

    return data


# this function prints the rows with dirty_bit 1 in given range and previous three rows foor the given start_date,end_date
def print_rows_start_date_end_date(start_date, end_date, data):
    session_id_start_end_time = {}  # map from session id to start_time-end_time; there is only one exchange so this works
    row_printed_to_index = {}        # map of row no printed to the index in fetched data;
    count_row_printed = 1
    count = 0
    lines_to_print = ""
    indep_start_time_end_time_to_print = []
    # load the map of session to start_time-end_time
    # fill the list of indep_start_time_end_time_to_print with the indep_start_time_end_time we would print
    # fill the list of all_red with all the row indexes with dirty_bit 1 and in the specified date range, which we would print in red
    for i in range(len(data)):
        dep_ = data[i][0]
        indep_ = data[i][1]
        session_id_ = data[i][2]
        date_generated_ = data[i][3]
        beta_ = data[i][4]
        correlation_ = data[i][5]
        dirty_bit_ = data[i][7]
        start_time_ = data[i][9]
        end_time_ = data[i][10]
        if session_id_ not in list(session_id_start_end_time.keys()):
            session_id_start_end_time[session_id_] = start_time_ + "_" + end_time_
        if dirty_bit_ == 1 and indep_ + "_" + start_time_ + "_" + end_time_ not in indep_start_time_end_time_to_print and (date_generated_) >= datetime.date(datetime.strptime(start_date, "%Y%m%d")):
            indep_start_time_end_time_to_print.append(indep_ + "_" + start_time_ + "_" + end_time_)
        if dirty_bit_ == 1:
            all_red.append(i)
    prev_dep_indep_session = data[0][0] + "^" + data[0][1] + "^" + data[0][9] + "^" + data[0][10]
    dep_indep_session_red = ""
    for i in range(len(data)):
        dep_ = data[i][0]
        indep_ = data[i][1]
        session_id_ = data[i][2]
        date_generated_ = data[i][3]
        beta_ = data[i][4]
        correlation_ = data[i][5]
        dirty_bit_ = data[i][7]
        start_time_ = data[i][9]
        end_time_ = data[i][10]
        if indep_ + "_" + start_time_ + "_" + end_time_ not in indep_start_time_end_time_to_print:
            continue
        dep_indep_session = dep_ + "^" + indep_ + "^" + start_time_ + "^" + end_time_
        # this condition seperates the blocks
        if dep_indep_session != prev_dep_indep_session:
            lines_to_print += "\n"
        # this condition checks if the dirty bit is 1 for a dep,indep,session combo and date greater than start_date and it has not been printed earlier; since the rows are sorted by dep,indep,session this condition holds
        if dirty_bit_ == 1 and date_generated_ >= datetime.date(datetime.strptime(start_date, "%Y%m%d")) and (dep_indep_session_red == "" or dep_indep_session_red != dep_ + "^" + indep_ + " " + start_time_ + "-" + end_time_):
            if count == 0:
                print("row_no dep^indep session date beta correlation dirty_bit \n")
            dep_indep_session_red = dep_ + "^" + indep_ + "^" + start_time_ + "^" + end_time_
            count += 1
            for j in range((i - 3), i):
                if dep_indep_session == data[j][0] + "^" + data[j][1] + "^" + data[j][9] + "^" + data[j][10]:
                    lines_to_print += ("    " + data[j][0] + "^" + data[j][1] + " " + data[j][9] + "-" + data[j][10] + " " + str(
                        data[j][3]) + " " + str(data[j][4]) + " " + str(data[j][5]) + " " + str(data[j][7]) + "\n")
            lines_to_print += ('\033[31m' + indent_row_no(count_row_printed) + " " + dep_ + "^" + indep_ + " " + start_time_ + "-" +
                               end_time_ + " " + str(date_generated_) + " " + str(beta_) + " " + str(correlation_) + " " + str(dirty_bit_) + "\033[39m" + "\n")
            row_printed_to_index[count_row_printed] = i
            count_row_printed += 1
            continue
            # after it has printed the prev three rows by the previous condition it continues to print all the rows for a combo in a block, in red or white depends on dirty_bit
        if dep_indep_session_red == dep_ + "^" + indep_ + "^" + start_time_ + "^" + end_time_:
            if dirty_bit_ == 1:
                lines_to_print += ('\033[31m' + indent_row_no(count_row_printed) + " " + dep_ + "^" + indep_ + " " + start_time_ + "-" +
                                   end_time_ + " " + str(date_generated_) + " " + str(beta_) + " " + str(correlation_) + " " + str(dirty_bit_) + "\033[39m" + "\n")
                row_printed_to_index[count_row_printed] = i
                count_row_printed += 1
            else:
                lines_to_print += ("    " + dep_ + "^" + indep_ + " " + start_time_ + "-" + end_time_ + " " +
                                   str(date_generated_) + " " + str(beta_) + " " + str(correlation_) + " " + str(dirty_bit_) + "\n")

        prev_dep_indep_session = dep_indep_session
    if count == 0:
        print("No dep indep pair found for the given date with dirty_bit =1")
        sys.exit()
    print(lines_to_print)
    return session_id_start_end_time, row_printed_to_index


def print_rows_date(date, data):
    session_id_start_end_time = {}
    row_printed_to_index = {}
    count_row_printed = 1
    count = 0
    lines_to_print = ""
    for i in range(len(data)):
        dep_ = data[i][0]
        indep_ = data[i][1]
        session_id_ = data[i][2]
        date_generated_ = data[i][3]
        beta_ = data[i][4]
        correlation_ = data[i][5]
        dirty_bit_ = data[i][7]
        start_time_ = data[i][9]
        end_time_ = data[i][10]
        if session_id_ not in list(session_id_start_end_time.keys()):
            session_id_start_end_time[session_id_] = start_time_ + "_" + end_time_
        if date == datetime.strftime(date_generated_, '%Y%m%d') and dirty_bit_ == 1:
            if count == 0:
                print("row_no dep^indep session date beta correlation dirty_bit \n ")
            dep_indep = dep_ + "^" + indep_
            for j in range(i + 3, i, -1):
                if dep_indep == data[j][0] + "^" + data[j][1] and data[j][9] == start_time_ and data[j][10] == end_time_:
                    lines_to_print += ("    " + data[j][0] + "^" + data[j][1] + " " + data[j][9] + "-" + data[j][10] + " " + str(
                        data[j][3]) + " " + str(data[j][4]) + " " + str(data[j][5]) + " " + str(data[j][7]) + "\n")
            lines_to_print += ('\033[31m' + indent_row_no(count_row_printed) + " " + dep_ + "^" + indep_ + " " + start_time_ + "-" +
                               end_time_ + " " + str(date_generated_) + " " + str(beta_) + " " + str(correlation_) + " " + str(dirty_bit_) + "\033[39m \n \n")
            row_printed_to_index[count_row_printed] = i
            count_row_printed += 1
            count += 1
            all_red.append(i)
    print(lines_to_print)
    if count == 0:
        print("No dep indep pair found for the given date with dirty_bit =1")
        sys.exit()
    return session_id_start_end_time, row_printed_to_index



# connect to the mysql server
conn = MySQLdb.connect(host="52.87.81.158", user="dvcwriter", passwd="f33du5rB", db="lrdb")
global cursor
cursor = conn.cursor()
global all_red
all_red = []
to_continue = 1
while to_continue:
    if start_date != None and end_date != None:
        data = get_data_start_date_end_date(start_date, end_date, cursor)
        session_id_start_end_time, row_printed_to_index = print_rows_start_date_end_date(start_date, end_date, data)
    elif date != None:
        data = get_data_date(date, cursor)
        session_id_start_end_time, row_printed_to_index = print_rows_date(date, data)
    else:
        print("You need to give either the date or both start_date and end_date")
        sys.exit()

    to_update, db_or_prev = take_arguments()
    if db_or_prev == 0:
        update_db_to_zero(data, date, start_date, end_date, session_id_start_end_time, row_printed_to_index, to_update)
    if db_or_prev == 1:
        update_db_to_prev(data, session_id_start_end_time, row_printed_to_index, to_update)

    to_continue = int(input("Do you want to continue 0/1 : "))

cursor.close()
connection().close()
