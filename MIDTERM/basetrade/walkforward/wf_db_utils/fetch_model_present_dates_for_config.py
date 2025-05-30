#!/usr/bin/env python
# I don't feel comfortable writing /home/dvctrader/anaconda2/bin/python for the interpreter
#
# fetch_strat_from_config_and_date.py - Fetch the latest date on which model was generated before a specified date for a given config from MySQL database
# Pseudocode :
# read args with argparse and call function
# In function establish SQL connection
# Run the query to get latest date on which model was generated from database where config and max date are what have been specified.

# import modules
from __future__ import print_function

import os
import sys

sys.path.append(os.path.expanduser('~/basetrade/'))
from walkforward.wf_db_utils.fetch_config_details import fetch_config_id
from walkforward.wf_db_utils.db_handles import connection
from walkforward.utils.sql_str_converter import sql_out_to_str


def fetch_model_present_dates_for_config(wf_config, start_date, max_date):
    """
    Returns the list of dates on which model was generated between a specified date range for a given config name.

    wf_config: str
                full path of the config that has to be read form the wf_config table

    start_date: int
                date on or after which the model dates needs to be checked

    max_date: int
                date on or before which the model dates needs to be checked

    returns: rows with date, modelid, model_coeffs, config_id

    """
    max_date = int(max_date)
    # Get basename of config
    wf_config = os.path.basename(wf_config)
    # Get config id from config name
    wf_config_id = fetch_config_id(wf_config)
    # prepare a cursor object using cursor() method
    cursor = connection().cursor()
    search_query = ("SELECT date, modelid, coeffs, configid FROM wf_model_coeffs WHERE configid = \"%s\" and date >= %d and date <= %d;" %
                    (wf_config_id, start_date, max_date))

    # execute the SQL query using execute() method.
    cursor.execute(search_query)
    # fetch all of the rows from the query
    data = cursor.fetchall()
    data = sql_out_to_str(data)

    return data
