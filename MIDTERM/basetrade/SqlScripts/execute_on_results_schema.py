#!/usr/bin/env python

import MySQLdb
import sys
import argparse
import json
import datetime
import os
import collections
import subprocess

sys.path.append(os.path.expanduser('~/basetrade/'))
from walkforward.wf_db_utils.db_handles import connection
from walkforward.definitions import execs
from walkforward.wf_db_utils.fetch_config_details import fetch_config_details
from walkforward.utils import date_utils
from walkforward.refresh_model import refresh_model

cursor = connection().cursor()

parser = argparse.ArgumentParser()
parser.add_argument('-n', dest='snippet', help="which snippet to execute", type=int, required=True)
args = parser.parse_args()


def find(name, path):
    for root, dirs, files in os.walk(path):
        if name in files:
            #print( os.path.join(root,name))
            return True
    return False


if args.snippet == 1:
    # get result count for all configs in DB
    t_date = datetime.date.today().strftime("%Y%m%d")
    t_ctype = 3
    sql_select = "select count(*) as n, c.configid, c.cname, c.config_type from wf_configs as c join wf_results on wf_results.configid = c.configid where c.config_type = %d group by configid order by n " % (t_ctype)
    cursor.execute(sql_select)
    data = cursor.fetchall()
    if len(data) > 0:
        for line in data:
            print(" ".join(str(x) for x in line))


if args.snippet == 2:
    # DB -> FileSystem
    # config exits on file system
    # model_list + param_list exists on file system

    # first get keys in DB
    # shortcode + start_time + end_time
    key_dd = collections.defaultdict(list)

    wf_prod_path = "/home/dvctrader/modelling/wf_strats/"
    wf_staged_path = "/home/dvctrader/modelling/wf_staged_strats/"

    sql_query = "select shortcode,start_time,end_time from wf_configs group by shortcode,start_time,end_time;"
    cursor.execute(sql_query)
    data = cursor.fetchall()
    if len(data) > 0:
        for line in data:
            print "processing key " + line[0] + " " + line[1] + " " + line[2]
            sql_query = "select cname from wf_configs where shortcode = \"%s\" and start_time = \"%s\" and end_time = \"%s\";" % (
                line[0], line[1], line[2])
            cursor.execute(sql_query)
            data1 = cursor.fetchall()
            if len(data1) > 0:
                for line in data1:
                    if not find(line[0], wf_prod_path) and not find(line[0], wf_staged_path):
                        print "config doesnt not exists " + line[0]

# if snippet == 3:
    # FileSystem -> DB
    # check of config in file system if it exists in DB

    # first get keys in filesystem
    # shortcode + start_time + end_time (from folder names ?)

# from today to walk_startdate do we have coeffs in the table ?

if args.snippet == 4:
    print("type 6 cleaning...")
    today = datetime.date.today().strftime("%Y%m%d")
    dates_cmd = [execs.execs().get_dates, "ZN_0", "20140101", str(today)]
    process = subprocess.Popen(' '.join(dates_cmd), shell=True,
                               stderr=subprocess.PIPE,
                               stdout=subprocess.PIPE)
    out, err = process.communicate()
    errcode = process.returncode
    if len(err) > 0:
        raise ValueError("Error in retrieving dates")

    dates = out.split()
    print(len(dates))
    sql_select = "select cname from wf_configs where config_type = 6 limit 1"
    cursor.execute(sql_select)
    data = cursor.fetchall()

    if len(data) > 0:
        for t_config in data:
            t_config = t_config[0]
            print(t_config)
            config_struct = fetch_config_details(t_config)
            config_json = json.loads(config_struct.config_json)
            expected_date_with_coeffs = today
            if config_json["trigger_string"] == "D0":
                for date in dates:
                    if int(date) > int(config_json["walk_start_date"]):
                        if int(date) < int(expected_date_with_coeffs):
                            expected_date_with_coeffs = date_utils.calc_this_prev_weekday_date(date, 0)
                            coeffs_sql_select = "SELECT MAX(date) FROM wf_model_coeffs WHERE configid =%d AND date <= %d;" % (
                                int(config_struct.configid), int(expected_date_with_coeffs))
                            cursor.execute(coeffs_sql_select)
                            data = cursor.fetchall()
                            if len(data) > 0:
                                actual_date_with_coeffs = data[0][0]
                                if actual_date_with_coeffs != expected_date_with_coeffs:
                                    print("for date: " + str(date) + " expected date: " + str(expected_date_with_coeffs) +
                                          " is matching actual date with coeffs " + str(actual_date_with_coeffs) + " refresing model")
                                    refresh_model(t_config, expected_date_with_coeffs, 1, day_to_model_param_pair, None)
                    else:
                        break
            if config_json["trigger_string"] == "M1":
                for date in dates:
                    if int(date) > int(config_json["walk_start_date"]):
                        if int(date) < int(expected_date_with_coeffs):
                            expected_date_with_coeffs = date_utils.calc_first_weekday_date_of_month(date)
                            coeffs_sql_select = "SELECT MAX(date) FROM wf_model_coeffs WHERE configid =%d AND date <= %d;" % (
                                int(config_struct.configid), int(expected_date_with_coeffs))
                            cursor.execute(coeffs_sql_select)
                            data = cursor.fetchall()
                            if len(data) > 0:
                                actual_date_with_coeffs = data[0][0]
                                if actual_date_with_coeffs != expected_date_with_coeffs:
                                    print("for date: " + str(date) + " expected date: " + str(expected_date_with_coeffs) +
                                          " is matching actual date with coeffs " + str(actual_date_with_coeffs) + " refresing model")
                                    refresh_model(t_config, expected_date_with_coeffs, 1, day_to_model_param_pair, None)
                    else:
                        break
