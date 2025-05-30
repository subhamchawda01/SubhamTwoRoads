#!/usr/bin/env python


"""
Removes the strategy for given config
"""
from __future__ import print_function

import sys
from datetime import datetime

# In Python 3 we should use pymysql
try:
    import pymysql
    pymysql.install_as_MySQLdb()
except ImportError:
    import MySQLdb
    pass

from walkforward.wf_db_utils.db_handles import connection
from walkforward.wf_db_utils.fetch_config_details import fetch_config_details


def remove_config_strats_from_db(cname, start_date, end_date):
    """

    Remove the daily strats from the wf_strats table for the given config_name and between the start and the end date


    cname: str
            The config name whose strats have to be removed


    start_date: int
            The start date from which the strat has to be removed


    end_date: int
            The end date till which the strat has to be removed


    returns: None




    """

    cfg = fetch_config_details(cname)
    if cfg.configid > 0:
        if cfg.config_type == 3:
            start_date = 19700101
        remove_configid_strats_from_db(cfg.configid, start_date, end_date)
    else:
        print("Invalid Configid for %s %d" % (cname, cfg.configid), file=sys.stderr)


def remove_configid_strats_from_db(configid, start_date, end_date):
    """

    Remove the daily strats from the wf_strats table for the given config_id and between the start and the end date

    configid: int
            The configid whose strats have to be removed


    start_date: int
            The start date from which the strat has to be removed


    end_date: int
            The end date till which the strat has to be removed


    returns: None



    """
    if end_date == 'TODAY':
        end_date = datetime.today().strftime('%Y%m%d')

    start_date = int(start_date)
    end_date = int(end_date)

    delete_query = 'DELETE FROM wf_strats where configid = %d AND date >= %d AND date <= %d' % (
        configid, start_date, end_date)

    try:
        connection().cursor().execute(delete_query)
    except MySQLdb.Error as e:
        print(("Could not execute %s" % delete_query, e), file=sys.stderr)
