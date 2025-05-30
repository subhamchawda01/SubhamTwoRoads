#!/usr/bin/env python


"""
Dumps the json string to database, it first check
"""
from __future__ import print_function

import os
import sys
import json
import MySQLdb

sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.wf_db_utils.db_handles import connection
from walkforward.wf_db_utils.fetch_dump_model import fetch_modelid_from_modelname
from walkforward.wf_db_utils.fetch_dump_param import fetch_paramid_from_paramname
from walkforward.utils.sql_str_converter import sql_out_to_str


def dump_strat_for_config_for_day(modelfilename, paramfilename, tradingdate, configname, overwrite=False):
    """
    Inserts the strategy corresponding to the model,param and date into the wf_strats table

    modelfilename:str
                    model name that fetches the model from the models table

    paramfilename:str
                    param name that fetches the param from the param table

    tradingdata: int
                    the date for which the wf_strats table is going to get updated

    configname: str
                    the config name that fetches the wf_config table

    overwrite: boolean
                    whether to overwrite the entry in the wf_strats table if there already exists an entry. Default is False


    return: None




    """
    # prepare a cursor object using cursor() method
    print("inside dump_strat_for_config_for_day")
    cursor = connection().cursor()
    fetch_query = ('SELECT configid FROM wf_configs WHERE cname =  \"%s\";' % (configname))
    cursor.execute(fetch_query)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    configid = None
    if len(data) > 0:
        configid = int(data[0][0])
    fetch_query = ('SELECT paramid modelid FROM wf_strats WHERE configid = %d AND date = %d' %
                   (configid, int(tradingdate)))
    cursor.execute(fetch_query)

    data = cursor.fetchall()
    data = sql_out_to_str(data)
    if len(data) > 0:
        if overwrite:
            modelid = fetch_modelid_from_modelname(modelfilename, True, configid)
            paramid = fetch_paramid_from_paramname(paramfilename, True, configid)
            update_query = 'UPDATE wf_strats SET paramid = %d, modelid = %d WHERE configid = %d AND date = %d'\
                           % (paramid, modelid, configid, int(tradingdate))
            try:
                cursor.execute(update_query)
            except MySQLdb.Error as e:
                print(("Could not Execute:\nQUERY: " + update_query, e))

        else:
            print("Already entry for config and data in wf_strats, skipping")
    else:
        print("dumping model and param into models and params")
        modelid = fetch_modelid_from_modelname(modelfilename, True, configid)
        paramid = fetch_paramid_from_paramname(paramfilename, True, configid)

        print("dumping strats in wf_strats")
        insert_query = 'INSERT INTO wf_strats (configid, date, paramid, modelid) VALUES (%d, %d, %d, %d)' \
                       % (configid, int(tradingdate), paramid, modelid)
        try:
            cursor.execute(insert_query)
        except MySQLdb.Error as e:
            print(("Could not Execute:\nQUERY: " + insert_query, e))


def update_strat_for_config_for_day(modelid, paramid, tradingdate, configid, overwrite=False):
    # prepare a cursor object using cursor() method
    cursor = connection().cursor()
    fetch_query = ('SELECT paramid modelid FROM wf_strats WHERE configid = %d AND date = %d' % (configid, tradingdate))
    cursor.execute(fetch_query)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    if len(data) > 0:
        if overwrite:
            update_query = 'UPDATE wf_strats SET paramid = %d, modelid = %d WHERE configid = %d AND date = %d'\
                           % (paramid, modelid, configid, tradingdate)
            try:
                cursor.execute(update_query)
            except MySQLdb.Error as e:
                print(("Could not Execute:\nQUERY: " + update_query, e))

        else:
            print("Already entry for config and data in wf_strats, skipping")
    else:
        insert_query = 'INSERT INTO wf_strats (configid, date, paramid, modelid) VALUES (%d, %d, %d, %d)' \
                       % (configid, tradingdate, paramid, modelid)
        try:
            cursor.execute(insert_query)
        except MySQLdb.Error as e:
            print(("Could not Execute:\nQUERY: " + insert_query, e))
