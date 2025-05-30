#!/usr/bin/env python

"""

Module used mostly by process_config,
provides support to do things for given config

"""
from __future__ import print_function

import os
import sys
import json
import shutil
import warnings

import MySQLdb

from walkforward.definitions import config
from walkforward.definitions.structured_config import StructuredConfig
from walkforward.definitions.defines import list_of_keys

from walkforward.wf_db_utils import fetch_config_details
from walkforward.wf_db_utils import dump_config_to_db
from walkforward.wf_db_utils import update_config_in_db
from walkforward.wf_db_utils.dump_structured_config_to_db import dump_structured_config_to_db
from walkforward.wf_db_utils.fetch_structured_config_details import fetch_structured_config_details
from walkforward.wf_db_utils.fetch_structured_config_details import fetch_structured_config_details_dicbt

from walkforward.wf_db_utils.delete_config_from_db import delete_config_from_db
from walkforward.wf_db_utils.remove_config_strats_from_db import remove_config_strats_from_db
from walkforward.wf_db_utils.remove_config_results_from_db import remove_config_results_from_db
from walkforward.wf_db_utils.cascade_delete import clear_configid_contents_from_schema
from walkforward.utils.file_utils import get_full_path
from walkforward.wf_db_utils.db_handles import connection
from walkforward.wf_db_utils.pool_utils import get_num_configs_for_pool
from walkforward.utils.sql_str_converter import sql_out_to_str
from walkforward.utils.update_type6_model_stdev import update_type6_model_stdev


def reload_config(configname, args):
    """

    :param configname: 
    :param args: 
    :return: 
    """
    if os.path.exists(configname):
        print('Reloading config in DB ' + configname)
        baseconfigname = os.path.basename(configname)

        with open(configname) as config_file:
            config_json = json.load(config_file)

        cfg = config.config.initialize()
        cfg.config_json = json.dumps(config_json, sort_keys=True)
        cfg.update_config_from_its_json()
        cfg.add_cname(baseconfigname)
        stratname = "INVALID"
        if args and len(args) > 0:
            stratname = args[0]
        cfg.add_sname(stratname)

        (exists, configid) = dump_config_to_db.dump_config_to_db(cfg, True)

        if exists == -1:
            print('No configname specified')
            exit(0)

        dump_config_to_db.dump_config_to_fs(cfg)
        print("Config added with configid " + str(configid))

    else:
        # for backward compatibility, we would want to fetch the file from modelling directory here
        print("Could not open configfile to add in database")
        exit(0)


def add_structured_config_to_database(configname, args):
    """

    :param configname: 
    :param args: 
    :return: 
    """

    baseconfigname = os.path.basename(configname)
    with open(configname) as config_file:
        config_json = json.load(config_file)

    cfg = StructuredConfig.initialize()
    cfg.config_json = json.dumps(config_json, sort_keys=True)

    cfg.update_config_from_its_json()
    cfg.add_cname(baseconfigname)
    cfg.is_structured = cfg.config_json["is_structured"]

    # dump the structured config to database
    (exists, configid) = dump_structured_config_to_db(cfg, False)

    if exists == -1:
        print('No Structured configname specified')
        exit(0)
    if exists == 0:
        print("Structured Config already exists in database, please change the mode")
        exit(0)

    if configid > 1:
        print('Structured Config addedd to database. Configid: ', configid)


def add_config_to_database(configname, args):
    """
    # this requires a valid pathname given for the config
    :param configname: 
    :param args: 
    :return: 
    """

    if os.path.exists(configname):
        print("Adding Config to database " + configname)
        baseconfigname = os.path.basename(configname)

        with open(configname) as config_file:
            config_json = json.load(config_file)

        if 'is_structured' in config_json and config_json['is_structured'] >= 1:
            add_structured_config_to_database(configname, args)
            return

        cfg = config.config.initialize()
        cfg.config_json = json.dumps(config_json, sort_keys=True)
        cfg.update_config_from_its_json()
        cfg.add_cname(baseconfigname)
        stratname = "INVALID"
        if args and len(args) > 0:
            stratname = args[0]
        cfg.add_sname(stratname)

        (exists, configid) = dump_config_to_db.dump_config_to_db(cfg, False)

        if exists == -1:
            print('No configname specified')
            exit(0)
        if exists == 0:
            print("Config already exists in database, please change the mode")
            exit(0)

        dump_config_to_db.dump_config_to_fs(cfg)
        print("Config added with configid " + str(configid))

    else:
        # for backward compatibility, we would want to fetch the file from modelling directory here
        print("Could not open configfile to add in database")
        exit(0)


def update_json_from_arg(cfg_json, args):
    """

    :param cfg_json: 
    :param args: 
    :return: 
    """
    for i in range(int(len(args) / 2)):
        key = args[2 * i]
        val = args[2 * i + 1]
        if key not in list_of_keys:
            print(key + " Doesn't seem to be a valid key. not updating. If you are adding new config-type, please update the, ")
            "file in ~/basetrade/walkforward/definitions/defines.py"
            return

        if key == 'param_list' or key == 'model_list' or key == 'sample_feature' or key == 'feature_switch_threshold':
            val = val.split(',')

        if val == 'DELETE':
            if key in cfg_json:
                del cfg_json[key]
            else:
                print("Could not find " + key + " in config to delete")
        else:
            cfg_json[key] = val


def modify_config_field(configname, params, args):
    """
    update the value in config
    :param configname: 
    :param params: 
    :param args: 
    :return: 
    """

    if not params or len(params) % 2 != 0:
        print("Please add both key-value pair for config")
        exit(0)

    baseconfigname = os.path.basename(configname)

    if args.is_structured == 1:
        cfg = fetch_structured_config_details(baseconfigname)
    elif args.is_structured == 3:
        cfg = fetch_structured_config_details_dicbt(baseconfigname)
    else:
        cfg = fetch_config_details.fetch_config_details(baseconfigname)

    if not cfg.config_json or cfg.config_json == 'INVALID':
        print("Config's json is invalid: " + cfg.config_json)
        exit(0)

    config_json = json.loads(cfg.config_json)
    # Checks if trying to udapte model stdev string. Works for only type 6 where model stdev was provided while creating config
    updating_stdev = False
    if 'model_process_string' in config_json.keys():
        if 'model_process_string' in params:
            model_process_string_index = params.index('model_process_string')
            model_process_list = list(map(float, config_json["model_process_string"].split()))
            previous_stdev_present = model_process_list[0]
            if previous_stdev_present != 0:
                previous_stdev = model_process_list[1]
                new_stdev_string_list = list(map(float, params[model_process_string_index + 1].split()))
                if new_stdev_string_list[0] == 0 or len(new_stdev_string_list) == 1:
                    print("Target Stdev not specified while updating config. Not updating model process string")
                    updating_stdev = False
                else:
                    updating_stdev = True
            else:
                print("Stdev was not specified while creating config. Not updating model process string")
                previous_stdev = "NA"
                updating_stdev = False

            if updating_stdev == False:  # Remove the model stdev updating params if they do not pass above criterion.
                del params[model_process_string_index]
                del params[model_process_string_index]

    if not params:
        print("Nothing to update. Exiting.")
        exit(0)

    update_json_from_arg(config_json, params)

    cfg.config_json = json.dumps(config_json, sort_keys=True)
    cfg.update_config_from_its_json()

#    print('PRINTING CONFIG')
#    cfg.print_config(True)

    # If we are editing config name, then updating baseconfigname variable.
    if 'cname' in params:
        cname_index = params.index('cname')
        baseconfigname = params[cname_index + 1]

    errcode_update = update_config_in_db.update_config_in_db(baseconfigname, cfg)

    if errcode_update != 0:
        print("Config Update Failed")
        exit(0)

    if updating_stdev:
        # Passing third argument as false, as the stdev is already updated in model process string in db
        update_type6_model_stdev(baseconfigname, previous_stdev, False)
        # If we are updating only model stdev, no need to clear strategies generated. Else we take them as provided in input
        if len(params) == 2:
            args.keep_strats = True

    # configname is config to be updated, baseconfigname can be new name if cname is being edited in this run.
    modify_config_field_in_filesystem(configname, baseconfigname, params)

    # if something is edited then wf_strats/models/params are not valid anymore
    if not args.keep_results:
        print("removing from wf_results")
        remove_results(baseconfigname, '19700101', 'TODAY')

    if not args.keep_strats:
        print(("clearing wf_strats, models and params " + str(cfg.configid)))
        clear_configid_contents_from_schema(cfg.configid)

        print("Inserting models in model_list into models table")
        dump_config_to_db.insert_into_modellist(cfg)

        print("Inserting params in param_list into params table")
        dump_config_to_db.insert_into_paramlist(cfg)

        dump_config_to_db.update_init_strats_map(cfg)


def modify_config_field_in_filesystem(configname, baseconfigname, args):
    """

    :param configname: 
    :param baseconfigname:
    :param args: 
    :return: 
    """
    full_path = configname
    if not os.path.exists(configname):
        full_path = get_full_path(os.path.basename(configname))

    if not full_path or not os.path.exists(full_path):
        print("The file doesn't exist in filesystem")
    else:
        with open(full_path) as flat_file:
            flat_file_json = json.load(flat_file)

        update_json_from_arg(flat_file_json, args)
        with open(full_path, 'w') as out_file:
            json.dump(flat_file_json, out_file)

        # In case, we are editing the config name, we need to rename file on filesystem.
        # In this case baseconfigname will be the new name passed thus will not match to basename(configname) as configname is old name.
        if baseconfigname != os.path.basename(configname):
            new_name_full_path = os.path.join(os.path.dirname(full_path), baseconfigname)
            os.system('mv ' + full_path + ' ' + new_name_full_path)


def print_config(configname, detailed):
    """

    :param configname: 
    :param detailed: 
    :return: 
    """
    baseconfigname = os.path.basename(configname)
    cfg = fetch_config_details.fetch_config_details(baseconfigname)

    if cfg:
        if cfg.is_valid_config():
            cfg.print_config(detailed)
        else:
            print("Config details are Invalid .. ")
            exit(0)
    else:
        print("Could not fetch the details of the config")
        exit(0)


def print_config_from_filesystem(configname):
    full_path = configname
    if not os.path.exists(configname):
        full_path = get_full_path(os.path.basename(configname))

    if not full_path or not os.path.exists(full_path):
        print("The file doesn't exist in filesystem")
    else:
        with open(full_path) as flat_file:
            flat_file_json = json.load(flat_file)
        modelling_cfg = config.config.initialize()
        modelling_cfg.config_json = json.dumps(flat_file_json, sort_keys=True)
        modelling_cfg.update_config_from_its_json()
        print("\n\nPrinting Config from modelling...\n")
        modelling_cfg.print_config()


def print_model_from_config(configname):
    """

    :param configname: 
    :return: 
    """
    baseconfigname = os.path.basename(configname)
    cfg = fetch_config_details.fetch_config_details(baseconfigname)

    if cfg:
        if cfg.is_valid_config():
            cfg.print_model()
            # print print_config_from_filesystem(configname)
        else:
            print('Config name are Invalid .. maybe no entry in database? ')
            exit(0)
    else:
        print('Could not fetch the details of the config')
        exit(0)


def print_param_from_config(configname):
    """

    :param configname: 
    :return: 
    """
    baseconfigname = os.path.basename(configname)
    cfg = fetch_config_details.fetch_config_details(baseconfigname)

    if cfg:
        if cfg.is_valid_config():
            cfg.print_param()
            # print print_config_from_filesystem(configname)
        else:
            print('Config name are Invalid .. maybe no entry in database? ')
            exit(0)
    else:
        print('Could not fetch the details of the config')
        exit(0)


def prune_config(configname, prune_in_filesystem=True):
    """

    Removes the entries related to the configname from the tables

    1. wf_config- config entry is removed from the wf_config table
    2. model   -  the model id for the given configname is removed from the models table
    3. params  -  the paramid for the given configname is removed from the params table

    configname:str
            the name of the config to be pruned
    prune_in_filesystem:boolean
            whether to remove the config from the file system

    return:None

    """
    basename = os.path.basename(configname)

    print("removing the config from the DB")
    errcode_db = delete_config_from_db(basename)
    if errcode_db != 0:
        print("Could not prune config in DB")
        return 1

    # only if the flag is set
    if prune_in_filesystem:
        full_name = os.path.abspath(configname)
        # if the file is not there, try finding it in the modelling
        if not os.path.exists(basename):
            full_name = get_full_path(basename)

        if not full_name or not os.path.exists(full_name):
            print("config doesn't exist on file system")
            return 1

        pool_dir = os.path.dirname(full_name)
        pruned_dir = pool_dir.replace('strats', 'pruned_strategies')
        pruned_path = pruned_dir + '/' + basename

        try:
            # create the prune directory
            if pruned_dir and not os.path.exists(pruned_dir):
                os.makedirs(pruned_dir)
            # move the file to pruned
            shutil.move(full_name, pruned_path)
        except EnvironmentError:
            print("Could not prune config in file system")
            exit(0)

        print(("PRUNED: " + full_name + " TO: " + pruned_path))


def remove_strats(configname, start_date, end_date):
    remove_config_strats_from_db(configname, start_date, end_date)


def remove_results(configname, start_date, end_date):
    remove_config_results_from_db(configname, start_date, end_date)


def update_pooltag(configname, pooltag):
    cfg = fetch_config_details.fetch_config_details(configname)
    if cfg.is_valid_config():
        cfg.pooltag = pooltag
        update_config_in_db.update_config_in_db(configname, cfg)
    else:
        print("Could not fetch details of the config %s" % configname)
        exit(1)


def update_expect0vol(configname, expect0vol):
    cfg = fetch_config_details.fetch_config_details(configname)
    if cfg.is_valid_config():
        cfg.expect0vol = expect0vol
        update_config_in_db.update_config_in_db(configname, cfg)
    else:
        print("Could not fetch details of the config %s" % configname)
        exit(1)


def move_to_pool_in_filesystem(configname):
    basename = os.path.basename(configname)
    full_name = os.path.abspath(configname)
    # if the file is not there, try finding it in the modelling
    if not os.path.exists(basename):
        full_name = get_full_path(basename)
    if not os.path.exists(full_name):
        print("Config doesnt exists on file system")
        return 1

    staged_dir = os.path.dirname(full_name)
    pool_dir = staged_dir.replace('wf_staged_strats', 'wf_strats')
    pool_path = pool_dir + '/' + basename

    try:
        # create the prune directory
        if pool_dir and not os.path.exists(pool_dir):
            os.makedirs(pool_dir)
        # move the file to pruned
        shutil.move(full_name, pool_path)
    except EnvironmentError:
        print("Could not move config in file system")
        return 1

    print(("MOVED: " + full_name + " TO: " + pool_path))
    return 0


def move_to_pool(configname):
    cfg = fetch_config_details.fetch_config_details(configname)
    if cfg.is_valid_config():
        num_configs = get_num_configs_for_pool(cfg.shortcode, cfg.start_time,
                                               cfg.end_time, 'N', cfg.pooltag, cfg.strat_type, cfg.event_token)

        # currently having restriction of max 30 strats in the pool
        if num_configs > 30:
            print("Can't move the config to prod pool!! Prod pool already has 30 configs")
            exit(1)
        else:
            # set the config type to normal here
            cfg.type = 'N'
            errcode_db = update_config_in_db.update_config_in_db(configname, cfg)
            if errcode_db != 0:
                print("Could not move the config in DB")
                exit(1)

            errcode_fs = move_to_pool_in_filesystem(configname)
            if errcode_fs != 0:
                exit(1)
    else:
        print(("Could not fetch details of the config %s" % configname))
        exit(1)
