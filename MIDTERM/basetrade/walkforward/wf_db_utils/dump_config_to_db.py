#!/usr/bin/env python


"""
Dumps the json string to database, it first check
"""

import sys
import os

import MySQLdb

from walkforward.wf_db_utils.db_handles import connection
from walkforward.wf_db_utils.cascade_delete import clear_configid_contents_from_schema, delete_config_from_configid
from walkforward.utils.sql_str_converter import sql_out_to_str
from walkforward.wf_db_utils.fetch_config_details import fetch_config_id
from walkforward.wf_db_utils.fetch_dump_model import fetch_modelid_from_modelname
from walkforward.wf_db_utils.fetch_dump_param import fetch_paramid_from_paramname
from walkforward.wf_db_utils.fetch_strat_from_config_and_date import fetch_strat_from_config_and_date
from walkforward.wf_db_utils.dump_strat_for_config_for_day import dump_strat_for_config_for_day
from walkforward.wf_db_utils.insert_model_into_db_wf_config_type6 import insert_model_coeffs_in_db
from walkforward.wf_db_utils import dump_model_list_to_db
from walkforward.wf_db_utils import dump_param_list_to_db


def insert_into_wfconfigs(config):
    """
    Insert the config details in the wf_config table

    config: config struct that has all the fields necessary to make it a valid config

    return: int configid

    """

    cursor = connection().cursor()

    if len(config.cname) > 255:
        raise ValueError("can not insert config " + config.cname + " .. filename too long")

    if config.event_token == 'INVALID' and config.strat_type == 'EBT':
        raise ValueError("invalid config " + config.cname + " with strat_type: " +
                         config.strat_type + " and event_token: " + config.event_token)

    if config.event_token != 'INVALID' and config.strat_type == 'Regular':
        raise ValueError("invalid config " + config.cname + " with strat_type: " +
                         config.strat_type + " and event_token: " + config.event_token)

    insert_query = 'INSERT INTO wf_configs (cname, shortcode, execlogic, start_time, end_time, sname, strat_type, ' \
                   'event_token, query_id, config_type, config_json, simula_approved, type, description, ' \
                   'pooltag, expect0vol, is_structured, structured_id) VALUES ( \"%s\", \"%s\", \"%s\", \"%s\", ' \
                   '\"%s\", \"%s\", \"%s\", \"%s\", %d, %d, \'%s\', %d, \"%s\", \"%s\", \"%s\", %d, %d, %d);' % \
                   (config.cname, config.shortcode, config.execlogic, config.start_time, config.end_time, config.sname, config.strat_type, config.event_token,
                    config.query_id, config.config_type, config.config_json, config.simula_approved, config.type, config.description,
                    config.pooltag, config.expect0vol, int(config.is_structured), int(config.structured_id))

    # fail id query id is not integer castable
    assert int(config.query_id)
    print(insert_query)
    print('\n')

    configid = -1

    try:
        cursor.execute(insert_query)
        configid = fetch_config_id(config.cname)
    except MySQLdb.Error as e:
        print(("Could not execute " + insert_query + " ERROR: " + str(e)))

    return configid


def insert_into_modellist(config):
    """
    Inserts the models mentioned in model_list in json of the config

    config: config struct that has all the fields necessary to make it a valid config

    return: list<int> modelids of those inserted

    """

    dump_model_list_to_db.insert_models(config.model_list(), config.configid, config)

    modelids = []
    for modelfilename in config.model_list():
        nmid = fetch_modelid_from_modelname(modelfilename, True, config.configid)
        modelids.append(nmid)

    print("MODEL IDs: {0}".format(','.join(list(map(str, modelids)))))
    return modelids


def insert_into_paramlist(config):
    """
    Inserts the params mentioned in model_list in json of the config

    config: config struct that has all the fields necessary to make it a valid config

    return: list<int> paramids of those inserted

    """

    dump_param_list_to_db.insert_params(config.param_list(), config.configid)

    paramids = []
    for paramfilename in config.param_list():
        npid = fetch_paramid_from_paramname(paramfilename, True, config.configid)
        paramids.append(npid)

    print('PARAM IDs: {0}'.format(' '.join(list(map(str, paramids)))))
    return paramids


def update_init_strats_map(config):
    """
    Inserts the initial param,model mapping for the config

    currently active for just type 3 and 6
    type3: this is the only mapping
    type6: this mapping corresponds to the walk_start_date
    """

    model_list = config.model_list()
    param_list = config.param_list()

    if len(model_list) < 1:
        print("WARN: config " + config.cname + " has no model mentioned..")
        return

    if len(param_list) < 1:
        print("WARN: config " + config.cname + " has no paramfile mentioned..")
        return

    if config.config_type == 3:
        tradingdate = 19700101

        print("inserting strat in wf_strats")
        dump_strat_for_config_for_day(model_list[0], param_list[0], tradingdate, config.cname)

    elif config.config_type == 6:
        model = model_list[0]
        param = param_list[0]
        tradingdate = int(config.get_json_value_for_key("walk_start_date"))

        print("Inserting strat in wf_strats init model and init param")
        dump_strat_for_config_for_day(model, param, tradingdate, config.cname)

        print("Inserting init models coeffs into  wf_model_coeffs")
        insert_model_coeffs_in_db(model, model, config.configid, tradingdate)


def dump_config_to_db(config, overwrite):
    """
    Insert the config details in the wf_config table
    Insert models in modellist in models table
    Insert params in paramlist in params table
    if type3, update into wf_strats
    if type6, update the wf_startdate mapping into wf_strats and modelcoeffs

    config: config struct that has all the fields necessary to make it a valid config

    overwrite: int
            whether to overwrite the config in the wf_config table if there is a config already present by this name 


    returns: None

    """

    if not config.is_valid_config(False):
        print('Config created is not valid')
        return -1, -1

    if not config.check_modelparam_exists_in_fs():
        print('Config created is not valid: some of the models/params do not exists in filesystem')
        return -1, -1

    if not config.cname:
        print("Config name not present")
        return -1, -1

    configid = fetch_config_id(config.cname)

    if configid is not None and not overwrite:
        print("There's already entry for config: " + config.cname)
        return (0, configid)

    elif configid is not None and overwrite:
        print(("Config exists and going for clean up in models, params, wf_strats, wf_model_coeffs. configid: " + str(
            configid)))
        delete_config_from_configid(configid)

    try:
        configid = insert_into_wfconfigs(config)
    except Exception as e:
        if configid and configid > 0:
            delete_config_from_configid(configid)
        raise ValueError("Error while inserting config " + config.cname + "\nError: " + str(e))

    config.configid = configid
    print("CONFIGID: " + str(configid))

    print("Inserting models in model_list into models table")
    try:
        insert_into_modellist(config)
    except Exception as e:
        delete_config_from_configid(configid)
        raise ValueError("Error while inserting models for config " + config.cname + "\nError: " + str(e))

    print("Inserting params in param_list into params table")
    try:
        insert_into_paramlist(config)
    except Exception as e:
        delete_config_from_configid(configid)
        raise ValueError("Error while inserting params for config " + config.cname + "\nError: " + str(e))

    try:
        update_init_strats_map(config)
    except Exception as e:
        delete_config_from_configid(configid)
        raise ValueError("Error while inserting initial wf_strats map for config " +
                         config.cname + "\nError: " + str(e))

    return (1, configid)


def dump_config_to_fs(config):
    # dumping the config to the filesystem in the staged strats directory
    staged_strats_directory = "/home/dvctrader/modelling/wf_staged_strats/" + \
        config.shortcode + "/" + config.start_time + "-" + config.end_time
    if not os.path.isdir(staged_strats_directory):
        os.system("mkdir -p " + staged_strats_directory)
    try:
        with open(os.path.join(staged_strats_directory, config.cname), 'w') as file:
            file.write(config.config_json)
    except:
        print ("Could not dump into file system at " + str(os.path.join(staged_strats_directory, config.cname)))
