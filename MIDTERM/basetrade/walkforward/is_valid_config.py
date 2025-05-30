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
from walkforward.wf_db_utils.db_handles import set_backtest
from walkforward.definitions import config


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
parser.add_argument('-c', dest='configname', help="the walk-forward-config identifier", type=str, required=True)

args = parser.parse_args()
if 'USE_BACKTEST' in os.environ:
    set_backtest(True)
print((is_valid_config(args.configname)))
