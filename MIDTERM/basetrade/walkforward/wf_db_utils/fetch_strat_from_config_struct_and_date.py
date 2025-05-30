#!/usr/bin/env python

"""

"""
import os
import json
import warnings
import getpass
import subprocess
from socket import gethostname
import pandas as pd


from walkforward.definitions import execs

from walkforward.wf_db_utils.db_handles import connection
from walkforward.wf_db_utils.fetch_config_details import fetch_config_name
from walkforward.wf_db_utils.fetch_config_details import fetch_config_details_from_configid
from walkforward.wf_db_utils.fetch_strat_from_type6_config_struct_and_date import fetch_strat_from_type6_config_struct_and_date
from walkforward.wf_db_utils.fetch_strat_from_type6_config_struct_and_date import fetch_strat_from_type6_config_struct_and_dates

from walkforward.utils.date_utils import *
from walkforward.utils.sql_str_converter import sql_out_to_str

from walkforward.utils.dump_content_to_file import write_model_to_file
from walkforward.utils.dump_content_to_file import write_param_to_file
from walkforward.utils.dump_content_to_file import write_model_param_files


def fetch_strat_from_config_struct_and_date(config_struct, trade_date, days_to_look=1):
    """

    This function takes the config and date and returns the corresponding strategy file for the day

     config_struct: config_struct
                        The config struct object

    trade_date : int 
                        The date for which the strat is required

    days_to_look: int
                        Number of lookback days. Default 1


    returns: tuple 

    (shortcode, execlogic, modelfilename, paramfilename, start_time, 
            end_time, strat_type, event_token, query_id)

    """

    if config_struct.config_type == 3:
        trade_date = 19700101

    if config_struct.config_type == 6 or config_struct.config_type == 7:
        num_lookbacks = min(4, days_to_look)
        return fetch_strat_from_type6_config_struct_and_date(config_struct, trade_date, num_lookbacks)

    cursor = connection().cursor()
    found = False
    num_days_so_far = 0
    while not found and num_days_so_far < days_to_look:
        # get the model, param name in this step
        search_query = (
            "SELECT modelfilename, paramfilename FROM wf_strats, models, params WHERE models.modelid = "
            "wf_strats.modelid AND params.paramid = wf_strats.paramid AND wf_strats.configid = %d AND date = %s" % (
                config_struct.configid, trade_date))
        cursor.execute(search_query)
        data = cursor.fetchall()
        data = sql_out_to_str(data)
        (modelfilename, paramfilename) = ("INVALID", "INVALID")
        if len(data) > 0:
            line = data[0]
            (modelfilename, paramfilename) = (line[0], line[1])
            found = True

        # else:
        #    print >>sys.stderr, "Could not find model,param for config " + wf_config + 'and date ' + str(trade_date)

        start_date_cmd = [execs.execs().calc_prev_week_day, str(trade_date), '1']

        # would not work with python 2.6
        out = subprocess.Popen(' '.join(start_date_cmd), shell=True, stdout=subprocess.PIPE)
        trade_date = int(out.communicate()[0].strip())
        num_days_so_far += 1

    if found == False :
        print(("could not fetch model and param for configid from wf_strats " + str(config_struct.configid)))
        return (config_struct.shortcode, config_struct.execlogic, modelfilename, paramfilename, config_struct.start_time,
                config_struct.end_time, config_struct.strat_type, config_struct.event_token, config_struct.query_id)
    
    temp_location = execs.get_temp_location()
    model_dir = temp_location + "/temp_models/"
    param_dir = temp_location + "/temp_params/"

    os.system("mkdir --parents " + model_dir)
    os.system("mkdir --parents " + param_dir)

    if os.path.isfile(modelfilename):
        refreshed_model_filename = modelfilename
    else:
        base_modelfilename = os.path.basename(modelfilename)
        refreshed_model_filename = model_dir + "w_model_" + str(config_struct.configid) + '_' + str(trade_date)
        write_model_to_file(refreshed_model_filename, modelfilename, None, False)

    if os.path.isfile(paramfilename):
        refreshed_param_filename = paramfilename
    else:
        base_paramfilename = os.path.basename(paramfilename)
        refreshed_param_filename = param_dir + base_paramfilename + "_" + str(trade_date)
        write_param_to_file(refreshed_param_filename, paramfilename)

    #print(refreshed_param_filename + " " + refreshed_model_filename)
    return (config_struct.shortcode, config_struct.execlogic, refreshed_model_filename, refreshed_param_filename, config_struct.start_time,
            config_struct.end_time, config_struct.strat_type, config_struct.event_token, config_struct.query_id)


def fetch_strat_from_config_struct_and_dates(config_struct, trade_date_list, days_to_look=1):
    """

    This function takes the config and date and returns the corresponding strategy file for the day

     config_struct: config_struct
                        The config struct object

    trade_date : list of int 
                        The list of dates for which the strat is required

    days_to_look: int
                        Number of lookback days. Default 1


    returns: dataframe

            with columns ['ConfigName','ConfigId','Date','StrategyLine']

    """

    if config_struct.config_type == 6 or config_struct.config_type == 7:
        #num_lookbacks = min(4, days_to_look)
        num_lookbacks = days_to_look
        return fetch_strat_from_type6_config_struct_and_dates(config_struct, trade_date_list, num_lookbacks)

    configid = config_struct.configid
    config_name = fetch_config_name(configid)
    cursor = connection().cursor()

    # If type 3, then just this date is required
    if config_struct.config_type == 3:
        all_dates_list = [19700101]
    else:
        all_dates_list = []
        earliest_day_to_look_for = calc_prev_week_day(min(trade_date_list), days_to_look)
        date = earliest_day_to_look_for
        while date <= max(trade_date_list):
            all_dates_list.append(date)
            date = calc_next_week_day(date, 1)

    if len(all_dates_list) == 1:
        tuple_list_dates = "(" + str(all_dates_list[0]) + ")"
    else:
        tuple_list_dates = tuple(all_dates_list)

    search_query = (
        "SELECT date, modelfilename, paramfilename FROM wf_strats, models, params WHERE models.modelid = "
        "wf_strats.modelid AND params.paramid = wf_strats.paramid AND wf_strats.configid = %d AND date in %s" % (

            configid, tuple_list_dates))
    cursor.execute(search_query)
    data = cursor.fetchall()
    data = sql_out_to_str(data)

    out_data = write_model_param_files(data, configid, config_name)

    df = pd.DataFrame(out_data, columns=['Date', 'ModelFileName', 'ParamFileName',
                                         'RefreshedModelFileName', 'RefreshedParamFileName'])

    output_list = []
    dates_model_present = df['Date'].tolist()
    for dt in trade_date_list:
        add_date_to_output = False
        if config_struct.config_type == 3:
            nearest_date = 19700101
            add_date_to_output = True
        else:
            model_present_before_curr_date_list = list(filter(lambda x: x <= dt, dates_model_present))
            if len(model_present_before_curr_date_list) != 0:
                nearest_date = max(model_present_before_curr_date_list)
                # Add strat if found model is not very far from current date
                if week_days_between_dates(nearest_date, dt) <= days_to_look:
                    add_date_to_output = True

        if add_date_to_output:
            filter_df = df[df['Date'] == nearest_date].copy()
            filter_df.loc[:, 'Date'] = dt
            output_list.append(filter_df)

    if len(output_list) != 0:
        out_df = pd.concat(output_list, ignore_index=True)
    else:
        return pd.DataFrame()

    out_df.loc[:, 'ConfigName'] = pd.Series([config_name] * out_df.shape[0])
    out_df.loc[:, 'ConfigId'] = pd.Series([configid] * out_df.shape[0])

    strategyline_prefix = "STRATEGYLINE"
    if config_struct.strat_type == "MRT":
        strategyline_prefix = "PORT_STRATEGYLINE"

    out_df.loc[:, "StrategyLine"] = list(map(lambda modelname, paramname: strategyline_prefix + " " + config_struct.shortcode + " " + config_struct.execlogic + " " + modelname + " " + paramname
                                             + " " + config_struct.start_time + " " + config_struct.end_time +
                                             " " + str(config_struct.query_id) + " " + config_struct.event_token,
                                             out_df['RefreshedModelFileName'], out_df['RefreshedParamFileName']))

    return out_df[['ConfigName', 'ConfigId', 'Date', 'StrategyLine']]
