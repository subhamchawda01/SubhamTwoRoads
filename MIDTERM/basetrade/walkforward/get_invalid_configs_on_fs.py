#!/usr/bin/env python

"""
This checks for validity of config,
for now adding just check for valid json string,
at later stage we can add more config-type based changes

"""


import argparse
import json
import sys
import os

sys.path.append(os.path.expanduser('~/basetrade/'))
# returns 0 if invalid, +ve number if valid

from walkforward.wf_db_utils import fetch_config_details
from walkforward.utils.get_all_files_in_dir import get_all_files_in_dir
from walkforward.definitions import config
from walkforward.definitions import execs


def is_valid_config(filename):
    configid = -1
    cfg = fetch_config_details.fetch_config_details(filename)
    if cfg.is_valid_config():
        configid = cfg.configid

    if configid > 0:
        return configid

    if os.path.exists(filename):
        try:

            with open(filename) as filehandle:
                json_struct = json.load(filehandle)
            if configid > 0:
                return configid
            else:
                return 0
        except:
            return -1
    elif configid > 0:
        # still returning 0 here as the file is not there in the filesystem
        return 0
    else:
        return -1


parser = argparse.ArgumentParser()
parser.add_argument('-p', dest='product', help="product folder to scan", type=str, required=True)
parser.add_argument('-t', dest='folder_type', help="staged/normal", type=str, required=False, default='N')

args = parser.parse_args()
config_list = []
if args.folder_type == "N":
    directory = execs.execs().modelling + "/wf_strats/" + args.product + "/"
    get_all_files_in_dir(directory, config_list)


if args.folder_type == "S":
    directory = execs.execs().modelling + "/wf_staged_strats/" + args.product + "/"
    get_all_files_in_dir(directory, config_list)

# print args.product + " " + str(len(config_list))
for config in config_list:
    if is_valid_config(config) == -1:
        print(config)
