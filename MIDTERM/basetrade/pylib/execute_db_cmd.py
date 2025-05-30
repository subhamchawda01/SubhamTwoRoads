#!/usr/bin/env python

"""
Execute some random cmd

"""

import sys
import MySQLdb

from pylib.pretty_print_dict import pretty_print_dict


def execute_db_cmd(prog, write_var, mode):
    db_user = "dvcreader"
    if write_var:
        db_user = "dvcwriter"

    # open a database connection
    # be sure to change the host IP address, username, password and database name to match your own
    if mode == 'results_db':
        connection = MySQLdb.connect(host="52.87.81.158", user=db_user, passwd="f33du5rB", db="results")
    elif mode == 'lrdb_db':
        db_user = ''
        connection = MySQLdb.connect(host="10.0.0.11", port=59999, user=db_user, passwd="", db="lrdb")
    elif mode == 'profilestats':
        # database for stats related to tradeinit/ors profiling.
        connection = MySQLdb.connect(host='52.87.81.158', user=db_user, passwd='f33du5rB', db='profilestats')
    else:
        sys.exit('mode not handled in execute_db_cmd function')

    # prepare a cursor object using cursor() method
    cursor = connection().cursor()
    # execute the SQL query using execute() method.
    cursor.execute(prog)
    # fetch all of the rows from the query
    if not write_var:
        data = cursor.fetchall()
        num_fields = len(cursor.description)
        field_names = [i[0] for i in cursor.description]

        tdict = {}
        colname2 = 'field_names'
        colname1 = 'data'
        tdict[colname2] = field_names
        tdict[colname1] = data

        # close the cursor object
        cursor.close()
        # close the connection
        connection.close()
        pretty_print_dict(tdict, colname1, colname2)
    else:
        connection.commit()
