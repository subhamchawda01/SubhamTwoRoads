#!/usr/bin/env python


"""
Utility to fetch and dump model details from DB
there is a primary key, unique key and two multple keys
modelid, modelfilename, shortcode and configid

so,
a) ADD
b) MODIFY (no function yet)
d) DELETE (put together with other tables, so we dont have dangling records)
d) FETCH/SELECT 


modelid, shortcode, modelfilename, regression, modelmath,
training_sd, training_ed, training_st, tradining_et
filter, pred_dur, pred_algo, sample_timeouts
stdev_or_l1norm, change_or_return, last_update_date
model_desc config
"""
from __future__ import print_function

import os
import shutil

import MySQLdb


from walkforward.wf_db_utils.db_handles import connection
from walkforward.utils.sql_str_converter import sql_out_to_str
from walkforward.wf_db_utils.fetch_config_details import fetch_config_details_from_configid
from walkforward.wf_db_utils import dump_model_list_to_db


def fetch_modelid_from_modelname(modelname, insert_if_not_found=False, config_id=None):
    """
    Gets the modelid for the given modlename

    modelname: str 
            modelname to get modelid for

    insert_if_not_found: boolean
            insert the model in the models table if not there already. Default False

    config_id: str/None 
            config_id corresponding to the modelname. Default None


    return:
         str
         modelid corresponding to the modelname


    """
    base_modelname = modelname

    # first fetch
    if config_id != None:  # mean exact filename should need not match if exists
        base_modelname = os.path.basename(modelname)
        if base_modelname.split("_")[-1] != str(config_id):
            base_modelname = base_modelname + "_" + str(config_id)

    fetch_query = ('SELECT modelid FROM models WHERE modelfilename = \"%s\";' % (base_modelname))

    # try getting for specific configid or NULL
    print(fetch_query)
    print('\n')
    cursor = connection().cursor()
    cursor.execute(fetch_query)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    modelid = -1

    if len(data) > 0:
        modelid = int(data[0][0])

    # if not already in DB, insert if flag is set and config_id is not None
    # config_id must be defined for models table
    elif insert_if_not_found and config_id is not None:

        print("no entry for model: " + base_modelname + " and configid " + str(config_id) + " inserting..")

        config = fetch_config_details_from_configid(config_id)
        dump_model_list_to_db.insert_model(modelname, config_id, config)

        modelid = fetch_modelid_from_modelname(base_modelname, False, config_id)
    else:
        print("No Entry for model " + base_modelname)

    return modelid


def fetch_modelname_from_modelid(modelid):
    """

    Returns the modelname given the modelid

    """
    fetch_query = ('SELECT modelfilename FROM models where modelid = %d ;' % (modelid))
    cursor = connection().cursor()
    cursor.execute(fetch_query)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    modelname = 'INVALID'
    if len(data) > 0:
        modelname = data[0][0]
    else:
        print("No entry for modelid " + str(modelid))

    return modelname


# Fetch the actual model stored in DB in the table 'models' in column 'model_desc'
def fetch_model_desc(modelid):
    """
    Get the model description from the modelid

    modelid: int
            modelid to fetch content for

    return:
        str
            model_desc - the model description for the provided modelid 

    """
    cursor = connection().cursor()
    model_desc = ""
    select_query = ('SELECT model_desc FROM models where modelid = %d ;' % (modelid))
    try:
        cursor.execute(select_query)
        data = cursor.fetchall()
        data = sql_out_to_str(data)
        if len(data) > 0:
            model_desc = data[0][0]
        else:
            model_desc = ""
    except MySQLdb.Error as e:
        print(("Could not execute the command %s " % (select_query), e))
    return model_desc


def fetch_model_desc_from_modelname(modelname):
    """
    Get the model description from the modelname

    """
    cursor = connection().cursor()
    model_desc = ""
    select_query = ('SELECT model_desc FROM models where modelfilename = \"%s\" ;' % (modelname))
    try:
        cursor.execute(select_query)
        data = cursor.fetchall()
        data = sql_out_to_str(data)
        if len(data) > 0:
            model_desc = data[0][0]
        else:
            model_desc = ""
    except MySQLdb.Error as e:
        print(("Could not execute the command %s " % (select_query), e))
    return model_desc


def fetch_model_desc_from_configid(config_id, index=0):
    cursor = connection().cursor()
    model_desc = ""
    select_query = ('SELECT model_desc from models where configid = %d;' % (config_id))
    try:
        cursor.execute(select_query)
        data = cursor.fetchall()
        data = sql_out_to_str(data)
        if len(data) > 0:
            model_desc = data[index][0]
        else:
            model_desc = ""
    except MySQLdb.Error as e:
        print(("Could not execute command %d " % (select_query), e))
    return model_desc


def fetch_model_desc_from_config_id_and_date(config_id, date):
    cursor = connection().cursor()
    model_id_sql_select = "SELECT modelid FROM wf_strats WHERE configid = %d AND date IN " \
        "(SELECT MAX(date) FROM wf_strats where configid=%d AND date<=%s)" % (
            int(config_id), int(config_id),
            int(date))

    try:
        cursor.execute(model_id_sql_select)
        data = cursor.fetchall()
        data = sql_out_to_str(data)
        if len(data) > 0:
            model_id = int(data[0][0])
        else:
            model_id = 0
        return fetch_model_desc(model_id)
    except MySQLdb.Error as e:
        print(("Could not execute command %d " % (model_id_sql_select), e))
    return ""


def fetch_model_id_from_config_id_and_date(modelFileName, config_id, date, insert_if_not_found=False):
    cursor = connection().cursor()

    fetch_query = ('SELECT modelid FROM wf_strats WHERE configid = %d AND date= %d;' % (config_id, date))

    # try getting for specific configid or NULL
    print(fetch_query)
    print('\n')
    cursor = connection().cursor()
    cursor.execute(fetch_query)
    data = cursor.fetchall()
    data = sql_out_to_str(data)

    base_modelname = os.path.basename(modelFileName)
    modelid = 0
    if len(data) > 0:
        modelid = int(data[0][0])

    # if not already in DB, insert if flag is set and config_id is not None
    # config_id must be defined for models table
    elif insert_if_not_found and config_id is not None:

        config = fetch_config_details_from_configid(config_id)
        dump_model_list_to_db.insert_model(modelFileName, config_id, config)

        modelid = fetch_modelid_from_modelname(base_modelname, False, config_id)
    else:
        print("No Entry for model " + base_modelname)

    return modelid


