#!/usr/bin/env python

"""
This script helps in updating modelling repo using data from DB.
"""

from __future__ import print_function

import os
import sys
import shutil
import json
import getpass

sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.wf_db_utils.db_handles import connection
from walkforward.utils.sql_str_converter import sql_out_to_str
from walkforward.wf_db_utils.pool_utils import get_pools_for_shortcode
from walkforward.wf_db_utils.pool_utils import get_configs_for_pool
from walkforward.definitions import execs

uname = getpass.getuser()

# function to execute query in the database and return results


def execute_query(query):
    cursor = connection().cursor()
    cursor.execute(query)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    return data


# makes directory if it does not exist
def make_dir(file_path):
    directory = os.path.dirname(file_path)
    if not os.path.exists(directory):
        os.makedirs(directory)


# wrapper function helps in printing to a file
def print_to_file(value, filename):
    try:
        file = open(filename, 'w+')
        if value is not None:
            file.write(value)
        file.close()
    except IOError as e:
        print("Error: could not create " + filename)

## to fetch content of models and params and write in files.
def update_modelling_repo(cname, base_dir = None, type_dir=None):
    if base_dir == None:
        base_dir = execs.execs().modelling_dvc
    if type_dir == None:
        type_dir = "wf_staged_strats"

    config_query = ('SELECT configid, config_json from wf_configs where cname = \"%s\";' % cname)
    config_data = execute_query(config_query)
    config_id = config_data[0][0]
    config_json = config_data[0][1]

    try:
        config_json_dict = json.loads(config_json)
    except:
        print("caught error for while loading json for cname: " + cname)
        return
    shc = config_json_dict["shortcode"]
    pool = config_json_dict["start_time"]+"-"+config_json_dict["end_time"]

    config_path = os.path.join(base_dir, type_dir, shc, pool, cname)
    make_dir(config_path)

    # query to find  models
    models_query = (
            'SELECT modelid, model_desc, modelfilename FROM models where configid = %d GROUP BY modelid' % config_id)
    models_data = execute_query(models_query)

    model_filename_list = []
    for model_line in models_data:
        model_id = model_line[0]
        model_desc = model_line[1]
        model_name = os.path.basename(model_line[2])

        # write model_desc to FS
        model_path = os.path.join(base_dir, 'models', shc, pool, model_name)
        make_dir(model_path)
        print_to_file(model_desc, model_path)
        model_filename_list.append(model_path)

    # query to find  params
    params_query = (
            'SELECT paramid, param_desc,paramfilename FROM params where configid = %d GROUP BY paramid' % config_id)
    params_data = execute_query(params_query)

    param_filename_list = []
    for param_line in params_data:
        param_id = param_line[0]
        param_desc = param_line[1]
        param_name = os.path.basename(param_line[2])

        # write param_desc to FS
        param_path = os.path.join(base_dir, 'params', shc, param_name)
        make_dir(param_path)
        print_to_file(param_desc, param_path)
        param_filename_list.append(param_path)

    config_json_dict["model_list"] = model_filename_list
    config_json_dict["param_list"] = param_filename_list
    config_json = json.dumps(config_json_dict, sort_keys=True)
    print_to_file(config_json, config_path)

    update_config_json_query = 'UPDATE wf_configs SET config_json = \'%s\' WHERE configid = %d' % (config_json, config_id)
    execute_query(update_config_json_query)



if __name__ == "__main__":

    # modelling directory
    base_dir = os.path.expanduser('~/modelling')

    # modelling backup directory
    base_dir_bkp = os.path.expanduser('~/modelling_bkp')
    if not os.path.exists(base_dir_bkp):
        os.mkdir(base_dir_bkp)

    # Main and Staged Pool Directories in FS
    pool_dirs = {'N': 'wf_strats', 'S': 'wf_staged_strats'}

    # Created backups of the existing wf_strats, wf_staged_strats, models, params
    # The backups are to be removed at end of the process
    dirs_to_move = ['wf_strats', 'wf_staged_strats', 'models', 'params']
    for dir in dirs_to_move:
        s_dirpath = os.path.join(base_dir, dir)
        t_dirpath = os.path.join(base_dir_bkp, dir)
        if os.path.exists(s_dirpath):
            print('moving ' + s_dirpath + ' to ' + t_dirpath)
            shutil.move(s_dirpath, t_dirpath)


    # Create fresh directories dumped from DB
    # for N (wf_strats) and S (wf_staged_strats)
    # Loop through all shortcodes
    # For each shortcode, process for
    # 1. Regular configs, 2. EBT configs
    # For each config, dump the models, params and the config_json

    # note: model_list and param_list in config_json
    #       are updated with the new paths

    for type, type_dir in pool_dirs.items():
        # query to fetch shortcodes for the main pool
        shc_query = ('SELECT DISTINCT shortcode FROM wf_configs WHERE type = \"%s\";' % type)
        shc_vec = execute_query(shc_query)
        shc_vec = [l[0] for l in shc_vec]

        for shc in shc_vec:
            pool_to_configs = dict()

            # Build Pool for Regular configs
            pools = get_pools_for_shortcode(shc, type, 'Regular')

            for tperiod in pools:
                tp_tokens = tperiod.split('-')
                if len(tp_tokens) < 2:
                    continue

                # pool is defined by start_time, end_time and/or pool_tag
                st, et = tp_tokens[0:2]
                pt = ''
                if len(tp_tokens) > 2:
                    pt = tp_tokens[2]

                pool_to_configs[tperiod] = get_configs_for_pool(shc, st, et, type, pt)

            # Build EBT pools
            ebt_pools = get_pools_for_shortcode(shc, type, 'EBT')

            for evtok in ebt_pools:
                ebtdir = 'EBT/' + evtok
                pool_to_configs[ebtdir] = get_configs_for_pool(shc, None, None, type, '', 'EBT', evtok)

            for pool, configlist in pool_to_configs.items():
                for cname in configlist:
                    update_modelling_repo(cname, base_dir, type_dir)



    # Remove backups of wf_strats, wf_staged_strats, models, params
    dirs_to_move = ['wf_strats', 'wf_staged_strats', 'models', 'params']
    for dir in dirs_to_move:
        bkp_dirpath = os.path.join(base_dir_bkp, dir)
        if os.path.exists(s_dirpath):
            if os.path.exists(bkp_dirpath):
                print('removing ' + bkp_dirpath)
                shutil.rmtree(bkp_dirpath)
