#!/usr/bin/env python

"""
this gets missing configs from 

"""


import argparse
import json
import sys
import os

sys.path.append(os.path.expanduser('~/basetrade/'))
# returns 0 if invalid, +ve number if valid

from walkforward.wf_db_utils import fetch_config_details
from walkforward.utils.get_all_files_in_dir import exists
from walkforward.definitions import config
from walkforward.definitions import execs
from walkforward.wf_db_utils.db_handles import connection


parser = argparse.ArgumentParser()
parser.add_argument('-p', dest='product', help="product folder to scan", type=str, required=True)
parser.add_argument('-t', dest='folder_type', help="staged/normal", type=str, required=False, default='N')

args = parser.parse_args()
select_sql = "select cname from wf_configs where shortcode = \"%s\" and  type = \"%s\"" % (
    args.product, args.folder_type)
cursor = connection().cursor()
cursor.execute(select_sql)
config_list = cursor.fetchall()

if args.folder_type == "N":
    directory = execs.execs().modelling + "/wf_strats/" + args.product + "/"
    for config in config_list:
        config = config[0]
        if not exists(config, directory):
            print(config)

if args.folder_type == "S":
    directory = execs.execs().modelling + "/wf_staged_strats/" + args.product + "/"
    for config in config_list:
        config = config[0]
        if not exists(config, directory):
            print(config)
