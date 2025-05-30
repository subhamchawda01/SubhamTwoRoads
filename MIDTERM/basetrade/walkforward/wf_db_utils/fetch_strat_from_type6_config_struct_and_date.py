#!/usr/bin/env python

"""
This is utility script to fetch the details of strategy of a type 6 config for a given date.
Fetching strat for type 6 config is different than 3/4/5 because, here we are changing the model weights while 
keeping the indicator (global set) same. This is 3 step process  
    a) fetch the indicator list 
    b) fetch the weights
    c) combine both
    
"""

import json
import getpass
import os
import warnings
from socket import gethostname
import pandas as pd

from walkforward.wf_db_utils.db_handles import connection
from walkforward.definitions import execs
from walkforward.utils.dump_content_to_file import write_model_to_file
from walkforward.utils.dump_content_to_file import write_param_to_file
from walkforward.utils.dump_content_to_file import write_model_param_files
from walkforward.utils import date_utils
from walkforward.wf_db_utils.fetch_config_details import fetch_config_details_from_configid
from walkforward.wf_db_utils.fetch_config_details import fetch_config_name
from walkforward.utils.sql_str_converter import sql_out_to_str


def fetch_strat_from_type6_config_struct_and_date(config_struct, trade_date, num_lookbacks=4):
    """
    As the name suggests, this is only for type 6 config.
    For a given config and trade_date, it returns the strategy file for that. This includes following steps : 
    a ) For the given config, fetch the ilist and weights
    b ) map the weights to ilists (create model)
    c ) fetch the params
    d ) write the params/models to shared location ( shared/ephemeral on ec2  and /spare/local on ny)
    e ) return the strategy path

    :param config_struct: Object of Config class 
    :param trade_date: given date
    :param num_lookbacks: earliest_day to look is last_trigger + num_lookbacks * periodicity
    :return: strategy filename
    """

    cursor = connection().cursor()
    found = False
    num_days_so_far = 0

    # basing on trigger we should have exact date for coeffs
    # if expected date is not present, we return for date <= expected_date
    expected_date_with_coeffs = trade_date
    earliest_day_to_look_for = trade_date
    config_json = json.loads(config_struct.config_json)

    if config_json["trigger_string"] == "D0":
        # expected date should be closest monday lookback
        expected_date_with_coeffs = date_utils.calc_this_prev_weekday_date(trade_date, 0)
        # earliest day to look for will be num_lookbacks weeks from expected day
        earliest_day_to_look_for = date_utils.calc_prev_day(expected_date_with_coeffs, num_lookbacks * 5)

    elif config_json["trigger_string"] == "M1":
        # expected date should be first weekday this month
        # we are using them exactly because that way we are sure our logic is working as expected, we cannot assume bugs here
        expected_date_with_coeffs = date_utils.calc_first_weekday_date_of_month(trade_date)
        # earliest day to look for will be num_lookbacks months from expected day
        earliest_day_to_look_for = date_utils.calc_prev_day(expected_date_with_coeffs, num_lookbacks * 25)

    else:
        # This is for trigger string of type STDEV_20_0.2.
        # Expected date can be trade date too.
        expected_date_with_coeffs = trade_date
        # for feature-trigger, no bar on earliest day
        earliest_day_to_look_for = int(config_json["walk_start_date"])

    configid = int(config_struct.configid)
    wf_stdate = int(config_json["walk_start_date"])
    config_name = fetch_config_name(configid)

    if int(expected_date_with_coeffs) < wf_stdate:
        expected_date_with_coeffs = wf_stdate
        earliest_day_to_look_for = wf_stdate

    (modelfilename, paramfilename, coeffs) = ("INVALID", "INVALID", "INVALID")
    actual_date_with_coeffs = None

    # we get modelid and paramid from wf_strats using configid and its walk_forward_start_date
    # we get modelfilename and paramfile correpondingly
    # then we get coeffs, using select coeffs from wf_model_coeffs where date in (select max(date) from wf_model_coeffs where modelid = 7 and date <= 20170404 );

    strat_sql_select = "SELECT date, modelid, paramid FROM wf_strats WHERE configid = %d AND date IN " \
                       "(SELECT MAX(date) FROM wf_strats where configid=%d AND date<=%s)" % (
                           int(configid), int(configid),
                           int(expected_date_with_coeffs))
    
    cursor.execute(strat_sql_select)
    data = cursor.fetchall()
    data = sql_out_to_str(data)

    if len(data) > 0:
        actual_date_with_coeffs = int(data[0][0])
        modelid = data[0][1]
        paramid = data[0][2]

        if actual_date_with_coeffs != expected_date_with_coeffs:
            warnings.warn("expected date " + str(expected_date_with_coeffs) + " and actual date " +
                          str(actual_date_with_coeffs) + " doesnt match, using the most recent "
                                                         "<= expected_date_with_coeffs for config " + config_name)
        if actual_date_with_coeffs < earliest_day_to_look_for:

            warnings.warn("Earliest day to look for: " + str(earliest_day_to_look_for) +
                          ". Found model for date: " + str(actual_date_with_coeffs) +
                          ". Quitting because the model is too old for config " + config_name)
            return (config_struct.shortcode, config_struct.execlogic, "INVALID",
                    "INVALID", config_struct.start_time,
                    config_struct.end_time, config_struct.strat_type, config_struct.event_token, config_struct.query_id)

        model_sql_select = "SELECT modelfilename FROM models WHERE modelid = %d" % (int(modelid))
        cursor.execute(model_sql_select)
        data = cursor.fetchall()
        data = sql_out_to_str(data)
        if len(data) > 0:
            modelfilename = data[0][0]
            param_sql_select = "SELECT paramfilename FROM params WHERE paramid = %d" % (int(paramid))
            cursor.execute(param_sql_select)
            data = cursor.fetchall()
            data = sql_out_to_str(data)
            if len(data) > 0:
                paramfilename = data[0][0]
                coeffs_sql_select = "SELECT date, coeffs FROM wf_model_coeffs WHERE date = %d AND " \
                                    "modelid = %d AND configid = %d" % (
                                        int(actual_date_with_coeffs), int(modelid), int(configid))
                # print coeffs_sql_select
                cursor.execute(coeffs_sql_select)
                data = cursor.fetchall()
                data = sql_out_to_str(data)
                if len(data) > 0:
                    actual_date_with_coeffs = data[0][0]
                    coeffs = data[0][1]

                else:
                    raise ValueError("Entry present in wf_strats but not in wf_model_coeffs. Check configid " +
                                     str(configid) + " modelid= " + str(modelid) + " and date= " + str(
                                         actual_date_with_coeffs))
            else:
                raise ValueError("No entry in params for paramid " + str(paramid))
        else:
            raise ValueError("No entry in models for modelid " + str(modelid))
    else:
        raise ValueError("no entry in wf_strats for configid, date " + str(configid) + " " + str(wf_stdate))

    # depending on the machine we are working on, get the directory for model path
    # TODO: please import the path from Execs/Paths class

    refreshed_model_filename = "INVALID"
    temp_location = execs.get_temp_location()
    model_dir = temp_location + "/trash/"
    param_dir = temp_location + "/trash/"

    os.system("mkdir --parents " + model_dir)
    os.system("mkdir --parents " + param_dir)

    base_modelfilename = os.path.basename(modelfilename)
    refreshed_model_filename = model_dir + "w_model_" + str(configid) + "_" + str(actual_date_with_coeffs)

    base_paramfilename = os.path.basename(paramfilename)
    refreshed_param_filename = param_dir + base_paramfilename + "_" + str(actual_date_with_coeffs)

    regress_exec = (config_json['reg_string'].split())[0]
    is_siglr = False
    if regress_exec == "SIGLR":
        is_siglr = True

    write_model_to_file(refreshed_model_filename, modelfilename, coeffs, is_siglr)
    write_param_to_file(refreshed_param_filename, paramfilename)

    return (config_struct.shortcode, config_struct.execlogic, refreshed_model_filename,
            refreshed_param_filename, config_struct.start_time,
            config_struct.end_time, config_struct.strat_type, config_struct.event_token, config_struct.query_id)


def check_strat_for_config6_and_date(config_id, trade_date, bypass=False):
    """
    As the name suggests, this is only for type 6 config.
    For a given configid and trade_date, it returns true if strategy exits (corresponding model/param is valid)


    :param config_id: int
    :param trade_date: given date 
    :param bypass: bypass the check for trade_date if True; for special case as tick_change_date
    :return: bool
    """

    cursor = connection().cursor()
    found = False
    num_days_so_far = 0
    config_struct = fetch_config_details_from_configid(config_id)

    # basing on trigger we should have exact date for coeffs
    # if expected date is not present, we return for date <= expected_date
    expected_date_with_coeffs = trade_date
    config_json = json.loads(config_struct.config_json)

    if config_json["trigger_string"] == "D0":
        # expected date should be closest monday lookback
        expected_date_with_coeffs = date_utils.calc_this_prev_weekday_date(trade_date, 0)

    elif config_json["trigger_string"] == "M1":
        # expected date should be first weekday this month
        # we are using them exactly because that way we are sure our logic is working as expected, we cannot assume bugs here
        expected_date_with_coeffs = date_utils.calc_first_weekday_date_of_month(trade_date)

    if (trade_date >= int(config_json['walk_start_date'])) and (expected_date_with_coeffs < int(config_json['walk_start_date'])):
        expected_date_with_coeffs = int(config_json['walk_start_date'])

    if bypass:
        expected_date_with_coeffs = trade_date
    configid = int(config_struct.configid)
    wf_stdate = int(config_json["walk_start_date"])

    (modelfilename, paramfilename, coeffs) = ("INVALID", "INVALID", "INVALID")
    actual_date_with_coeffs = None

    # we get modelid and paramid from wf_strats using configid and its walk_forward_start_date
    # we get modelfilename and paramfile correpondingly
    # then we get coeffs, using select coeffs from wf_model_coeffs where date in (select max(date) from wf_model_coeffs where modelid = 7 and date <= 20170404 );

    strat_sql_select = "SELECT date, modelid, paramid FROM wf_strats WHERE configid = %d AND date IN " \
                       "(SELECT MAX(date) FROM wf_strats where configid=%d AND date<=%s)" % (
                           int(configid), int(configid),
                           int(expected_date_with_coeffs))
    cursor.execute(strat_sql_select)
    data = cursor.fetchall()
    data = sql_out_to_str(data)

    if len(data) > 0:
        actual_date_with_coeffs = int(data[0][0])
        modelid = data[0][1]
        paramid = data[0][2]

        if actual_date_with_coeffs != expected_date_with_coeffs:
            return (False, expected_date_with_coeffs)

        model_sql_select = "SELECT modelfilename FROM models WHERE modelid = %d" % (int(modelid))
        cursor.execute(model_sql_select)
        data = cursor.fetchall()
        data = sql_out_to_str(data)
        if len(data) > 0:
            modelfilename = data[0][0]
            param_sql_select = "SELECT paramfilename FROM params WHERE paramid = %d" % (int(paramid))
            cursor.execute(param_sql_select)
            data = cursor.fetchall()
            data = sql_out_to_str(data)
            if len(data) > 0:
                paramfilename = data[0][0]
                coeffs_sql_select = "SELECT date, coeffs FROM wf_model_coeffs WHERE date = %d AND " \
                                    "modelid = %d AND configid = %d" % (
                                        int(actual_date_with_coeffs), int(modelid), int(configid))
                
                cursor.execute(coeffs_sql_select)
                data = cursor.fetchall()
                data = sql_out_to_str(data)
                if len(data) > 0:
                    actual_date_with_coeffs = data[0][0]
                    coeffs = data[0][1]

                else:
                    return False, expected_date_with_coeffs
            else:
                return False, expected_date_with_coeffs
        else:
            return False, expected_date_with_coeffs
    else:
        return False, expected_date_with_coeffs

    if modelfilename != "INVALID" and paramfilename != "INVALID" and coeffs != "INVALID":
        return True, expected_date_with_coeffs
    else:
        return False, expected_date_with_coeffs


def fetch_strat_from_type6_config_struct_and_dates(config_struct, trade_date_list, num_lookbacks=4):
    """
    As the name suggests, this is only for type 6 config.
    For a given config and list of trade_date, it returns the strategy file for each of them. This includes following steps :
    a ) For the given config, fetch the ilist and weights
    b ) map the weights to ilists (create model)
    c ) fetch the params
    d ) write the params/models to shared location ( shared/ephemeral on ec2  and /spare/local on ny)
    e ) return the strategy path

    :param config_struct: Object of Config class
    :param trade_date: list of given int dates
    :param num_lookbacks: earliest_day to look is last_trigger + num_lookbacks * periodicity
    :return: Dataframe
             Columns as ['ConfigName','ConfigId','Date','StrategyLine']
    """

    cursor = connection().cursor()

    min_trade_date = min(trade_date_list)
    config_json = json.loads(config_struct.config_json)
    configid = int(config_struct.configid)
    wf_stdate = int(config_json["walk_start_date"])
    config_name = fetch_config_name(configid)

    if config_json["trigger_string"] == "D0":
        # earliest day to look for will be num_lookbacks weeks from expected day
        days_to_look = num_lookbacks * 5
    elif config_json["trigger_string"] == "M1":
        # earliest day to look for will be num_lookbacks months from expected day
        days_to_look = num_lookbacks * 25
    else:
        # for feature-trigger, no bar on earliest day
        days_to_look = 100  # Can be changed

    earliest_day_to_look_for = date_utils.calc_prev_week_day(min_trade_date, days_to_look)
    all_dates_list = []
    date = earliest_day_to_look_for
    while date <= max(trade_date_list):
        all_dates_list.append(date)
        date = date_utils.calc_next_week_day(date, 1)

    expected_date_with_coeffs_dict = {}
    # For all dates, get the expected trigger dates
    for dt in all_dates_list:
        if config_json["trigger_string"] == "D0":
            # expected date should be closest monday lookback
            expected_date_with_coeffs_dict[dt] = date_utils.calc_this_prev_weekday_date(dt, 0)
        elif config_json["trigger_string"] == "M1":
            # expected date should be first weekday this month
            # we are using them exactly because that way we are sure our logic is working as expected, we cannot assume bugs here
            expected_date_with_coeffs_dict[dt] = date_utils.calc_first_weekday_date_of_month(dt)
        else:
            # This is for trigger string of type STDEV_20_0.2.Expected date can be trade date too.
            expected_date_with_coeffs_dict[dt] = dt

    # Get the set of dates for which we need to query the db. These can be only the expected dates.
    candidate_dates_model_present = list(set(expected_date_with_coeffs_dict.values()))

    if len(candidate_dates_model_present) == 1:
        tuple_list_dates = "(" + str(candidate_dates_model_present[0]) + ")"
    else:
        tuple_list_dates = tuple(candidate_dates_model_present)

    search_query = ("SELECT wf_strats.date, modelfilename, paramfilename, wf_model_coeffs.coeffs "
                    "FROM wf_strats, models, params, wf_model_coeffs "
                    "WHERE wf_strats.modelid = models.modelid AND wf_strats.paramid = params.paramid "
                    "and wf_model_coeffs.modelid = models.modelid and wf_model_coeffs.configid = wf_strats.configid and wf_model_coeffs.date = wf_strats.date "
                    "AND wf_strats.configid = %s AND wf_strats.date in %s" % (
                        config_struct.configid, tuple_list_dates))
    cursor.execute(search_query)
    data = cursor.fetchall()
    data = sql_out_to_str(data)

    regress_exec = (config_json['reg_string'].split())[0]
    is_siglr = False
    if regress_exec == "SIGLR":
        is_siglr = True

    out_data = write_model_param_files(data, configid, config_name, coeffs_present=True, is_siglr=is_siglr)

    df = pd.DataFrame(out_data, columns=['Date', 'ModelFileName', 'ParamFileName',
                                         'ModelCoeffs', 'RefreshedModelFileName', 'RefreshedParamFileName'])

    output_list = []
    dates_model_present = df['Date'].tolist()
    for dt in trade_date_list:
        model_present_before_curr_date_list = list(filter(lambda x: x <= dt, dates_model_present))
        if len(model_present_before_curr_date_list) != 0:
            nearest_date = max(model_present_before_curr_date_list)
            if date_utils.week_days_between_dates(nearest_date, dt) <= days_to_look:
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
