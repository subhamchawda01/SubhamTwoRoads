#!/usr/bin/env python
# I don't feel comfortable writing /home/dvctrader/anaconda2/bin/python for the interpreter
#
# fetch_strat_from_config_and_date.py - Fetch the model and param corresponding to the from a MySQL database
# Pseudocode :
# read args with argparse and call function
# In function establish SQL connection
# Get the model and param from database where config and tradedate are what have been specified.
# In case of a failure, try previous tradedate and return value. Keep trying previous dates for 100 times. TODO : I am not certain if this shuld be the correct failure hanlding.
# In case of any failure, even if recovered, log it somewhere. Perhaps log it locally right now.

# import modules
from __future__ import print_function
import os
import pdb
import pandas as pd
from walkforward.definitions import config
from walkforward.definitions.db_tables_defines import wf_configs_search_query
from walkforward.utils.sql_str_converter import sql_out_to_str
from walkforward.wf_db_utils.db_handles import connection
from walkforward.wf_db_utils.read_config_fields_from_data import read_config_fields_from_data



def fetch_config_details(wf_config):
    """

    Returns the config struct from the config name . Looks for the config details in the DB.

    wf_config: str
                full path of the config that has to be read form the wf_config table

    returns:config_struct


    """
    wf_config = os.path.basename(wf_config)
    # prepare a cursor object using cursor() method
    cursor = connection().cursor()
    search_query = wf_configs_search_query + 'cname = \"%s\";' % wf_config

    # print search_query

    # execute the SQL query using execute() method.
    cursor.execute(search_query)
    # fetch all of the rows from the query
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    # print the rows
    config_struct = config.config.initialize()
    config_struct.add_cname(wf_config)
    # print config_struct

    if len(data) > 0:
        config_struct = read_config_fields_from_data(config_struct, data[0])

    return config_struct


def fetch_config_details_from_configid(config_id):
    """

    Returns the config struct from the configid . Looks for the config details in the DB.

    wf_config: str
                full path of the config that has to be read form the wf_config table

    returns:config_struct


    """
    # prepare a cursor object using cursor() method
    cursor = connection().cursor()
    search_query = wf_configs_search_query + ' configid = %d' % config_id

    # print search_query

    # execute the SQL query using execute() method.
    cursor.execute(search_query)
    # fetch all of the rows from the query
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    # print the rows
    config_struct = config.config.initialize()
    # print config_struct

    if len(data) > 0:
        config_struct = read_config_fields_from_data(config_struct, data[0])

    return config_struct


def fetch_config_id(wf_config):
    """
    Get the config_id from the config name


    wf_config:str
                full path of the config that has to be read form the wf_config table

    return: str
                config id for the provided config
    """
    configid = None
    wf_config = os.path.basename(wf_config)
    # prepare a cursor object using cursor() method
    cursor = connection().cursor()
    sql_select = ("SELECT configid FROM  wf_configs WHERE cname = \"%s\";" % (wf_config))
    cursor.execute(sql_select)
    data = cursor.fetchall()
    data = sql_out_to_str(data)

    if len(data) > 0:
        line = data[0]
        configid = line[0]

    return configid

def fetch_configs_id(wf_configs):
    """
    Get the config_id list from the base config name list

    wf_config:list                

    return: list
                config id list for the provided config
    """
    # prepare a cursor object using cursor() method
    cursor = connection().cursor()
    wf_configs = tuple(wf_configs)
    params = {'wf_configs' : wf_configs}
    sql_select = ("SELECT cname, configid FROM  wf_configs WHERE cname in %(wf_configs)s;" %(params))
    cursor.execute(sql_select)
    names = [x[0] for x in cursor.description]
    rows = list(cursor.fetchall())
    data = pd.DataFrame(rows, columns=names)
    return(data)


def fetch_config_name(config_id):
    """
    Get the configname from the config id

    config_id: int or str
                id of config in database

    return: str
                config name for the provided config id
    """
    config_name = None
    if config_id is None:
        return config_name
    configid = int(config_id)
    # prepare a cursor object using cursor() method
    cursor = connection().cursor()
    sql_select = ("SELECT cname FROM wf_configs WHERE configid = %d;" % (configid))
    cursor.execute(sql_select)
    data = cursor.fetchall()
    data = sql_out_to_str(data)

    if len(data) > 0:
        line = data[0]
        config_name = line[0]

    return config_name

def fetch_configs_strat_type(wf_configs):
    """
    Get the config_id list from the base config name list
    wf_config:list                
    return: list
                strat_type , CBT/MRT/Regular/ list for the provided config

    strat_type Regular|EBT|MRT|
    if futher is_structured = 0, call it REG/EBT/MRT
              is_structured = 1,
              if exec = LFITradingManager-StructuredGeneralTrading-PriceBasedAggressiveProRataTrading : LFITM
              if exec = PriceBasedAggressiveProRataTrading : CBT
              is_structured = 2, call it None
              is_structured = 3, call it DI1TM

    exec_logic
    """
    # prepare a cursor object using cursor() method
    cursor = connection().cursor()
    wf_configs = tuple(wf_configs)
    params = {'wf_configs' : wf_configs}
    sql_select = ("SELECT strat_type, is_structured, execlogic FROM  wf_configs WHERE cname in %(wf_configs)s;" %(params))
    cursor.execute(sql_select)
    data = cursor.fetchall()
    data = sql_out_to_str(data) # this will be list 
    t_size = len(data)
    config_type = []
    for i in range(t_size):
        if data[i][1] == 0:
            if data[i][0] == 'Regular':
                config_type.append('REG')
            else:
                config_type.append(data[i][0])
        elif data[i][1] == 1:
            if data[i][2] == 'LFITradingManager-StructuredGeneralTrading-PriceBasedAggressiveProRataTrading':
                config_type.append('LFITM')
            else:
                config_type.append('CBT')
        elif data[i][1] == 2:
            config_type.append('')
        elif data[i][1] == 3:
            config_type.append('DI1TM')
        else:
            config_type.append('')
    return(config_type)

