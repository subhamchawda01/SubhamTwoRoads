#!/usr/bin/env python


# fetch_prev_model_data_for_config_and_date.py - Fetch the model data for the model which was generated before a specified date for a given config from MySQL database
# Pseudocode :
# In function establish SQL connection
# Run the query to get model data for the model which was generated before a specified date from database where config and max date are what have been specified.

# import modules
from __future__ import print_function

import os
import sys

sys.path.append(os.path.expanduser('~/basetrade/'))
from walkforward.wf_db_utils.fetch_config_details import fetch_config_id
from walkforward.wf_db_utils.db_handles import connection
from walkforward.utils.sql_str_converter import sql_out_to_str


def fetch_prev_model_data_for_config_and_date(wf_config, max_date):
    """
    Returns the list of dates on which model was generated between a specified date range for a given config name.

    wf_config: str
                base config name of the config that has to be read form the wf_config table

    max_date: int
                date before which the model dates needs to be fetched
                
    returns: row with date, modelid, model_coeffs, config_id
    
    """
    max_date = int(max_date)
    # Get basename of config
    wf_config = os.path.basename(wf_config)
    # Get config id from config name
    wf_config_id = fetch_config_id(wf_config)
    # prepare a cursor object using cursor() method
    cursor = connection().cursor()
    search_query = ("SELECT date, modelid, coeffs, configid FROM wf_model_coeffs WHERE configid = \"%s\" and date < %d order by date desc limit 1;" %
                    (wf_config_id, max_date))

    # execute the SQL query using execute() method.
    cursor.execute(search_query)
    # fetch all of the rows from the query
    data = cursor.fetchall()
    data = sql_out_to_str(data)

    return data