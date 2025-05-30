#!/usr/bin/env python

"""
Fetch the structured strategy components and return the string to print

"""

import os
import sys
import time

from walkforward.definitions.execs import paths

from walkforward.wf_db_utils.fetch_structured_config_details import fetch_structured_config_details
from walkforward.wf_db_utils.fetch_strat_from_config_struct_and_date import fetch_strat_from_config_struct_and_date
from walkforward.wf_db_utils.fetch_structured_config_details import fetch_structured_config_details_dicbt_for_date
from walkforward.wf_db_utils.db_handles import connection
from walkforward.utils.sql_str_converter import sql_out_to_str
from walkforward.definitions.structured_config import StructuredConfig
import pandas as pd


def fetch_structured_strat_from_name_and_date(configname, trade_date, days_to_look):
    """

    :param configname: 
    :param trade_date: 
    :param days_to_look: 
    :return: 
    """
    wf_config = os.path.basename(configname)
    wf_config = os.path.basename(configname)
    # prepare a cursor object using cursor() method
    cursor = connection().cursor()
   # #search_query = wf_configs_search_query + "cname = \"%s\";" % wf_config
    #print("WF config :")
    # print(wf_config)
    search_query = "SELECT is_structured FROM wf_configs WHERE cname = \"%s\" " % (wf_config)
    #print("Search Query :")
    # print(search_query)

    try:
        cursor.execute(search_query)
    except:
        print(sys.stderr, 'Could not execute query ' + search_query)

    # fetch all of the rows from the query
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    is_structured = 0
    if len(data) > 0:
        is_structured = data[0][0]

    config_struct = StructuredConfig.initialize()
    config_struct.add_cname(wf_config)

    if is_structured == 3:
        config_struct = fetch_structured_config_details_dicbt_for_date(wf_config, trade_date, days_to_look)
        config_struct.sanitize()
    else:
        config_struct = fetch_structured_config_details(wf_config)
        config_struct.sanitize()

    return fetch_structured_strat_from_config_struct_and_date(config_struct, trade_date, days_to_look)


def fetch_structured_strat_from_config_struct_and_date(config_struct, trade_date, days_to_look):
    """

    :param config_struct: 
    :param date: 
    :param days_to_look: 
    :return: 
    """
    t1 = time.time()
    

    if not config_struct.is_structured:
        print(sys.stderr, 'Strategy is not structured. Please call appropriate function')
        return ""

    

    (shortcode, execlogic, common_modelfilename, common_paramfilename,
     start_time, end_time, strat_type, event_token, query_id) = fetch_strat_from_config_struct_and_date(config_struct, int(trade_date), days_to_look)
    if shortcode is None:
        return None

    string_to_print = 'STRUCTURED_TRADING %s %s %s %s %s %d \n' % (
        shortcode, execlogic, common_paramfilename, start_time, end_time, query_id)

    for config in config_struct.config_vector:
        (shortcode, execlogic, modelfilename, paramfilename,
         start_time, end_time, strat_type, event_token, query_id) = fetch_strat_from_config_struct_and_date(config, int(trade_date), days_to_look)
        string_to_print += 'STRATEGYLINE %s %s %s %s %s %s \n' % (
            shortcode, modelfilename, paramfilename, start_time, end_time, config.cname)

    strategy_path_dir = paths().fbpa_user + 'temp_im_strats/'

    if not os.path.exists(strategy_path_dir):
        os.mkdir(strategy_path_dir, 755)

    strategy_path = strategy_path_dir + 'im_strat_' + config_struct.cname + '_' + str(trade_date)

    im_strat_file = open(strategy_path, 'w')
    im_strat_file.write(string_to_print)
    im_strat_file.close()

    # return the path for im_strat
    t2 = time.time()

    return 'STRUCTURED_STRATEGYLINE ' + strategy_path + ' ' + str(query_id)


def fetch_structured_strat_from_name_and_dates(configname, trade_date_list, days_to_look=1, is_structured=1):
    
    wf_config = os.path.basename(configname)
    outputlist = []
    for trade_date in trade_date_list:
        config_struct = StructuredConfig.initialize()
        config_struct.add_cname(wf_config)

        if is_structured == 3:
            config_struct = fetch_structured_config_details_dicbt_for_date(wf_config, trade_date, days_to_look)
            config_struct.sanitize()
        else:
            config_struct = fetch_structured_config_details(wf_config)
            config_struct.sanitize()
        
        strategyline = fetch_structured_strat_from_config_struct_and_date(config_struct, trade_date, days_to_look)

        if strategyline is not None:
            outputlist.append([wf_config, config_struct.configid, trade_date, strategyline])
    if len(outputlist) != 0:
        output_df = pd.DataFrame(outputlist, columns=['ConfigName', 'ConfigId', 'Date', 'StrategyLine'])
        return output_df[['ConfigName', 'ConfigId', 'Date', 'StrategyLine']]
    else:
        return pd.DataFrame()
