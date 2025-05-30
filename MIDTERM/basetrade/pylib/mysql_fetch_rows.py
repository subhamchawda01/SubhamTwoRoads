#!/usr/bin/env python

"""
mysql_fetch_rows.py - Fetch and display the rows from a MySQL database query

"""

import os
import sys
import argparse

sys.path.append(os.path.expanduser('~/basetrade/'))


from pylib.execute_db_cmd import execute_db_cmd

if len(sys.argv) > 1:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cmd", "-c", help="Command to execute")
    parser.add_argument("--write", "-w", help="Write/Update to DB")
    args = parser.parse_args()
    given_prog = args.cmd
    write_var = (args.write != None)

    if given_prog == None:
        print("-c should not be empty")
    else:
        execute_db_cmd(given_prog, write_var, 'results_db')
