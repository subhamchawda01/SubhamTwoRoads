#!/usr/bin/env python

"""
This script has utilities to insert dates from a date file/ list to the DB, corresponding to a configid
"""

import os
import json
import datetime

import MySQLdb
from walkforward.wf_db_utils.db_handles import connection


def get_dates_in_diff_format(date_list):
    if date_list == []:
        return ""
    date_list_unique = list(set(date_list))
    date_list_unique.sort()
    date_list_unique = [datetime.datetime.strptime(x, "%Y%m%d") for x in date_list_unique]
    diff_type_list = [0] * len(date_list_unique)
    for i in range(len(date_list_unique) - 1, 0, -1):
        delta_string = str(date_list_unique[i] - date_list_unique[i - 1]).strip().split()[0]
        diff_type_list[i] = delta_string

    diff_type_list[0] = date_list_unique[0].strftime('%Y%m%d')

    date_diff_string_to_insert = ",".join(diff_type_list)
    return date_diff_string_to_insert


def insert_date_in_diff_format(configid, training_dates_list, validation_dates_list, testing_dates_list, preprocessing_dates_list):

    training_dates_string = get_dates_in_diff_format(training_dates_list)
    validation_dates_string = get_dates_in_diff_format(validation_dates_list)
    testing_dates_string = get_dates_in_diff_format(testing_dates_list)
    preprocessing_dates_string = get_dates_in_diff_format(preprocessing_dates_list)

    db_table = 'training_testing_days'

    insert_query = ("INSERT IGNORE INTO " + db_table + "(configid,training_dates_in_diff,validation_dates_in_diff,testing_dates_in_diff,preprocessing_dates_in_diff" +
                    ") VALUES (" + str(configid) + ",'" + training_dates_string + "','" + validation_dates_string +
                    "','" + testing_dates_string + "','" + preprocessing_dates_string + "')")

    print(insert_query)
    try:
        cursor = connection().cursor()
        cursor.execute(insert_query)
        connection().commit()
    except MySQLdb.Error as e:
        raise ValueError("could not insert the dates due to error : ", e)
