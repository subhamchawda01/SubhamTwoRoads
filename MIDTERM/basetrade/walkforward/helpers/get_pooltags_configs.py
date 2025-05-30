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


def get_directory(name, path):
    for root, dirs, files in os.walk(path):
        if name in files:
            #print( os.path.join(root,name))
            return os.path.basename(root)
    return ''


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
        cdir = get_directory(config, directory)
        if cdir != '':
            cdir_toks = cdir.split('-')
            if len(cdir_toks) > 2:
                select_sql = "update wf_configs set pooltag = \"%s\" where cname = \"%s\"" % (cdir_toks[2], config)
                cursor.execute(select_sql)
                # print select_sql

if args.folder_type == "S":
    directory = execs.execs().modelling + "/wf_staged_strats/" + args.product + "/"
    for config in config_list:
        config = config[0]
        cdir = get_directory(config, directory)
        if cdir != '':
            cdir_toks = cdir.split('-')
            if len(cdir_toks) > 2:
                select_sql = "update wf_configs set pooltag = \"%s\" where cname = \"%s\"" % (cdir_toks[2], config)
                cursor.execute(select_sql)


connection().commit()
