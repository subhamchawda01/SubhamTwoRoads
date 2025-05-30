#!/usr/bin/env python


"""
there is a primary key, unique key and a multple key
paramid, paramfilename, configid

so,
a) ADD
b) MODIFY (no function yet)
d) DELETE (put together with other tables, so we dont have dangling records)
d) FETCH/SELECT 


paramid, paramfilename, last_update_date, param_desc and configid
"""
from __future__ import print_function

import os
import json
import datetime

import MySQLdb

from walkforward.wf_db_utils.db_handles import connection


def insert_params(param_list_filenames, config_id):
    for paramfile in set(param_list_filenames):
        insert_param(paramfile, config_id)


def insert_param(paramfile, config_id):
    """
    inserts param and dumps its contents into DB
    paramfilename: basename + config_id
    param_desc should not none/empty
    :param paramfile:
    :param config_id:
    :return:
    """

    try:
        with open(paramfile, 'r') as mfh:
            param_desc = mfh.read()
            mfh.close()
    except:
        raise ValueError(paramfile + " cannot be opened")

    pfn_2_insert = os.path.basename(paramfile)
    if pfn_2_insert.split("_")[-1] != str(config_id):
        pfn_2_insert = pfn_2_insert + "_" + str(config_id)

    if len(pfn_2_insert) > 255:
        raise ValueError("can not insert paramfile " + pfn_2_insert + " .. filename too long")

    if not param_desc or not pfn_2_insert:
        raise ValueError("can not insert param with empty name or empty param_desc")

    today = datetime.date.today().strftime("%Y%m%d")
    insert_query = "INSERT INTO params(paramfilename, last_update_date, param_desc, configid) VALUES"\
                   "(\"%s\", %s, \"%s\", %s);"\
                   % (pfn_2_insert, today, param_desc, str(config_id))

    print(insert_query)
    print('\n')
    try:
        cursor = connection().cursor()
        cursor.execute(insert_query)
    except MySQLdb.Error as e:
        raise ValueError("could not insert param the command", e)


# insert
# if inserting we need config_id
# we send filesystem_filename

# fetch
# fullmodelfilename or basefilename + configid
