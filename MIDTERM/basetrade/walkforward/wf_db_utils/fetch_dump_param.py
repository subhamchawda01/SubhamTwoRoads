#!/usr/bin/env python


"""
utility to dump and fetch model and param into the filename
"""
from __future__ import print_function

import os
import shutil

import MySQLdb

from walkforward.wf_db_utils.db_handles import connection
from walkforward.utils.sql_str_converter import sql_out_to_str
from walkforward.wf_db_utils import dump_param_list_to_db


def fetch_paramid_from_paramname(paramname, insert_if_not_found=False, config_id=None):
    """


    Gets the paramid for the given pramname

    paramname: str 
            paramname to get paramid for

    insert_if_not_found: boolean
            insert the param in the params table if not there already. Default False

    config_id: str/None 
            config_id corresponding to the paramname. Default None


    return:
         str
         paramid corresponding to the modelname



    """

    # check if the config_id is not already added to the param name ,if not then add, this is to avoid double appending of config_id
    base_paramname = paramname
    if config_id != None:
        base_paramname = os.path.basename(base_paramname)
        if base_paramname.split("_")[-1] != str(config_id):
            base_paramname = base_paramname + "_" + str(config_id)

    fetch_query = ('SELECT paramid FROM params WHERE paramfilename = \"%s\";' % (base_paramname))

    print(fetch_query)
    cursor = connection().cursor()
    cursor.execute(fetch_query)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    paramid = -1

    if len(data) > 0:
        paramid = int(data[0][0])

    # if not already in DB, insert if flag is set and config_id is not None
    # config_id must be defined for params table
    elif insert_if_not_found and config_id is not None:

        print("no entry for param: " + base_paramname + " and configid " + str(config_id) + " inserting")

        dump_param_list_to_db.insert_param(paramname, config_id)

        paramid = fetch_paramid_from_paramname(base_paramname, False, config_id)
    else:
        print("Could not find entry for param %s" % (base_paramname))

    return paramid


def fetch_paramname_from_paramid(paramid):
    """

    Returns the paramname from the paramid

    paramid: int
               the param id whose param name is to be fetch

    returns: str
            paramname: the name of the param corresponding to the paramid

    """
    fetch_query = ('SELECT paramfilename FROM params where paramid = %d' % (paramid))
    cursor = connection().cursor()
    cursor.execute(fetch_query)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    paramname = 'INVALID'

    if len(data) > 0:
        paramname = data[0][0]
    else:
        print("No entry for paramid " + str(paramid))

    return paramname


def fetch_param_desc_from_paramname(paramname):
    cursor = connection().cursor()
    param_desc = ""
    select_query = ('SELECT param_desc FROM params where paramfilename = \"%s\" ;' % (paramname))
    try:
        cursor.execute(select_query)
        data = cursor.fetchall()
        data = sql_out_to_str(data)
        if len(data) > 0:
            param_desc = data[0][0]
        else:
            param_desc = ""
    except MySQLdb.Error as e:
        print(("Could not execute the command %s " % (select_query), e))
    return param_desc


def fetch_param_desc_from_configid(configid, index=0):
    cursor = connection().cursor()
    param_desc = ""
    select_query = ('SELECT param_desc FROM params where configid = %d ;' % (configid))
    try:
        cursor.execute(select_query)
        data = cursor.fetchall()
        data = sql_out_to_str(data)
        if len(data) > 0:
            param_desc = data[index][0]
        else:
            param_desc = ""
    except MySQLdb.Error as e:
        print(("Could not execute the command %s " % (select_query), e))
    return param_desc


def fetch_paramid_from_paramname(paramname, insert_if_not_found=False, config_id=None):
    """


    Gets the paramid for the given pramname

    paramname: str
            paramname to get paramid for

    insert_if_not_found: boolean
            insert the param in the params table if not there already. Default False

    config_id: str/None
            config_id corresponding to the paramname. Default None


    return:
         str
         paramid corresponding to the modelname



    """

    # check if the config_id is not already added to the param name ,if not then add, this is to avoid double appending of config_id
    base_paramname = paramname
    if config_id != None:
        base_paramname = os.path.basename(base_paramname)
        if base_paramname.split("_")[-1] != str(config_id):
            base_paramname = base_paramname + "_" + str(config_id)

    fetch_query = ('SELECT paramid FROM params WHERE paramfilename = \"%s\";' % (base_paramname))

    print(fetch_query)
    cursor = connection().cursor()
    cursor.execute(fetch_query)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    paramid = -1

    if len(data) > 0:
        paramid = int(data[0][0])

    # if not already in DB, insert if flag is set and config_id is not None
    # config_id must be defined for params table
    elif insert_if_not_found and config_id is not None:

        print("no entry for param: " + base_paramname + " and configid " + str(config_id) + " inserting")

        dump_param_list_to_db.insert_param(paramname, config_id)

        paramid = fetch_paramid_from_paramname(base_paramname, False, config_id)
    else:
        print("Could not find entry for param %s" % (base_paramname))

    return paramid


def fetch_param_id_from_config_id_and_date(paramFileName, config_id, date, insert_if_not_found=False):
    cursor = connection().cursor()

    fetch_query = ('SELECT paramid FROM wf_strats WHERE configid = %d AND date= %d;' % (config_id, date))

    # try getting for specific configid or NULL
    print(fetch_query)
    print('\n')
    cursor = connection().cursor()
    cursor.execute(fetch_query)
    data = cursor.fetchall()
    data = sql_out_to_str(data)

    base_paramname = os.path.basename(paramFileName)

    if len(data) > 0:
        paramid = int(data[0][0])

    # if not already in DB, insert if flag is set and config_id is not None
    # config_id must be defined for models table
    elif insert_if_not_found and config_id is not None:

        print("no entry for param: " + base_paramname + " and configid " + str(config_id) + " inserting")
        dump_param_list_to_db.insert_param(paramFileName, config_id)

        paramid = fetch_paramid_from_paramname(base_paramname, False, config_id)
    else:
        print("No Entry for model " + base_paramname)

    return paramid
