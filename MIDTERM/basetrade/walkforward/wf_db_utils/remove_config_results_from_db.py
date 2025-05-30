#!/usr/bin/env python

"""
Remove the config results from database

"""
from __future__ import print_function

import sys

import MySQLdb


from datetime import datetime
from walkforward.definitions import config

from walkforward.wf_db_utils.db_handles import connection
from walkforward.wf_db_utils.fetch_config_details import fetch_config_details


def remove_config_results_from_db(cname, start_date, end_date):
    """

    Delete the results for a given config from the wf_results table for the period between start and the end data


    cname: str
            name of the config whose results have to be removed

    start_date: int
            the start date from which the result has to be removed

    end_date: int
            the end date till which the resut has to be removed



    returns:None




    """

    cfg = fetch_config_details(cname)
    remove_configid_results_from_db(cfg.configid, start_date, end_date)


def remove_configid_results_from_db(configid, start_date, end_date):
    """

    Delete the results for a given config from the wf_results table for the period between start and the end data


    configid:int
             configid whose results have to be removed

    start_date: int
            the start date from which the result has to be removed

    end_date: int
            the end date till which the resut has to be removed


    returns: None


    """

    if end_date == 'TODAY':
        end_date = datetime.today().strftime('%Y%m%d')

    start_date = int(start_date)
    end_date = int(end_date)

    delete_query = 'DELETE FROM wf_results WHERE configid = %d AND date >= %d AND date <= %d' % (
        configid, start_date, end_date)
    cursor = connection().cursor()
    try:
        cursor.execute(delete_query)
    except MySQLdb.Error as e:
        print(("Failed Execute Command %s" % delete_query, e), file=sys.stderr)
