#!/usr/bin/env python
"""
Fetch and create struct for structured strategy

"""

import os
import sys
import warnings
import json
from walkforward.utils import date_utils

from walkforward.utils.dump_content_to_file import write_model_to_file
from walkforward.utils.dump_content_to_file import write_param_to_file
from walkforward.definitions.structured_config import StructuredConfig
from walkforward.definitions.config import config

from walkforward.definitions.db_tables_defines import wf_configs_search_query

from walkforward.utils.sql_str_converter import sql_out_to_str


from walkforward.wf_db_utils.read_config_fields_from_data import read_config_fields_from_data
from walkforward.wf_db_utils.read_config_fields_from_data_DICBT import read_config_fields_from_data_DICBT
from walkforward.wf_db_utils.db_handles import connection
from walkforward.wf_db_utils.fetch_config_details import fetch_config_name
from walkforward.definitions import execs


def fetch_structured_config_details(configname):
    """
    Return the config details of given configname from DB
    :param configname: 
    :return: 
    """
    wf_config = os.path.basename(configname)
    # prepare a cursor object using cursor() method
    cursor = connection().cursor()
    search_query = wf_configs_search_query + "cname = \"%s\";" % wf_config

    # print search_query

    # execute the SQL query using execute() method.
    try:
        cursor.execute(search_query)
    except:
        print(sys.stderr, 'Could not execute query ' + search_query)

    # fetch all of the rows from the query
    data = cursor.fetchall()
    data = sql_out_to_str(data)

    config_struct = StructuredConfig.initialize()
    config_struct.add_cname(wf_config)

    if len(data) > 0:
        config_struct = read_config_fields_from_data(config_struct, data[0])

    if not config_struct.is_structured:
        print(sys.stderr, 'Config provided is not structured. Exiting..\n')
    else:
        # now fetch the sub strats
        config_json = json.loads(config_struct.config_json)
        search_query = wf_configs_search_query + ' structured_id = %d' % config_struct.configid

        try:
            cursor.execute(search_query)
        except:
            print(sys.stderr, 'Could not execute query' + search_query)
        data = sql_out_to_str(cursor.fetchall())

        for line in data:
            # read each line from data one by one and add that to structured config
            local_struct = config.initialize()
            local_struct = read_config_fields_from_data(local_struct, line)
            config_struct.config_vector.append(local_struct)

    return config_struct


def fetch_structured_config_details_dicbt(configname):
    """
        Return the config details of given configname from DB
        :param configname:
        :return:
        """
    wf_config = os.path.basename(configname)
    # prepare a cursor object using cursor() method
    cursor = connection().cursor()
    search_query = wf_configs_search_query + "cname = \"%s\";" % wf_config

    # print search_query

    # execute the SQL query using execute() method.
    try:
        cursor.execute(search_query)
    except:
        print(sys.stderr, 'Could not execute query ' + search_query)

    # fetch all of the rows from the query
    data = cursor.fetchall()
    data = sql_out_to_str(data)

    config_struct = StructuredConfig.initialize()
    config_struct.add_cname(wf_config)

    if len(data) > 0:
        config_struct = read_config_fields_from_data(config_struct, data[0])

    if not config_struct.is_structured:
        print(sys.stderr, 'Config provided is not structured. Exiting..\n')
    else:
        # now fetch the sub strats
        config_json = json.loads(config_struct.config_json)
        max_child_shortcodes = config_json['max_child_shortcodes']
        child_cname_prefix = config_struct.cname + "_child_"
        search_query = wf_configs_search_query + ' cname LIKE %s' % child_cname_prefix
        try:
            cursor.execute(search_query)
        except:
            print(sys.stderr, 'Could not execute query' + search_query)
        data = sql_out_to_str(cursor.fetchall())
        for line in data:
            # read each line from data one by one and add that to structured config
            local_struct = config.initialize()
            local_struct = read_config_fields_from_data(local_struct, line)
            config_struct.config_vector.append(local_struct)

    return config_struct


def fetch_structured_config_details_dicbt_for_date(configname, trade_date, num_lookbacks=4):
    """
        Return the config details of given configname from DB
        :param configname:
        :return:
        """
    wf_config = os.path.basename(configname)
    # prepare a cursor object using cursor() method
    cursor = connection().cursor()
    search_query = wf_configs_search_query + "cname = \"%s\";" % wf_config

    # print search_query

    # execute the SQL query using execute() method.
    try:
        cursor.execute(search_query)
    except:
        print(sys.stderr, 'Could not execute query ' + search_query)

    # fetch all of the rows from the query
    data = cursor.fetchall()
    data = sql_out_to_str(data)

    config_struct = StructuredConfig.initialize()
    config_struct.add_cname(wf_config)

    if len(data) > 0:
        config_struct = read_config_fields_from_data(config_struct, data[0])

    #cursor = connection().cursor()

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

    f = open(refreshed_model_filename, "r")
    model = f.read()

    if not config_struct.is_structured:
        print(sys.stderr, 'Config provided is not structured. Exiting..\n')
    else:
        # now fetch the sub strats
        config_json = json.loads(config_struct.config_json)
        max_child_shortcodes = config_json['max_child_shortcodes']
        #print("MODEL : ")
        # print(model)
        #print("Length of Config Struct :")
        # print(len(config_struct.config_vector))
        #print("Length of Model :")
        # print(len(model.strip().split("\n")))
        for index in range(0, len(model.strip().split("\n"))):
            shortcode = model.strip().split("\n")[index].split()[2]
            child_cname_prefix = config_struct.cname + "_child_" + str(index)
            search_query = wf_configs_search_query + ' cname = \"%s\"' % child_cname_prefix
            try:
                cursor.execute(search_query)
            except:
                print(sys.stderr, 'Could not execute query' + search_query)
            data = sql_out_to_str(cursor.fetchall())
            for line in data:
                # read each line from data one by one and add that to structured config
                local_struct = config.initialize()
                local_struct = read_config_fields_from_data_DICBT(local_struct, line, shortcode)
                config_struct.config_vector.append(local_struct)
    #print("Length of Config Struct :")
    # print(len(config_struct.config_vector))
    return config_struct
