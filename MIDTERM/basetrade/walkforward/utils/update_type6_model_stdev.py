from __future__ import print_function
import pdb
import os,io
import sys
import time
import getpass
import argparse
import json
import subprocess
import glob
import numpy as np
import pandas as pd
import datetime as dt
sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.wf_db_utils.fetch_config_details import fetch_config_details
from walkforward.wf_db_utils.fetch_model_present_dates_for_config import fetch_model_present_dates_for_config
from walkforward.wf_db_utils.fetch_dump_coeffs import insert_or_update_model_coeffs

def update_type6_model_stdev(config_file, stdev, is_target_stdev = True):
    """

    :param config_file:         base config name
    :param stdev:               value of stdev.  If it is previous stdev or target stdev is decided by the next argument
    :param is_target_stdev:     True by default. If True, previous stdev is picked from config json and target stdev is 'stdev' argument provided
                                                 If False,target stdev is picked from config json and previous stdev is 'stdev' argument provided
    :return: 
    """

    if stdev == "NA":
        print("Stdev was not specified correctly. Can't rescale")
        return False

    wf_config = os.path.basename(config_file)

    config_struct = fetch_config_details(wf_config)
    config_struct.sanitize()

    config_id = int(config_struct.configid)
    config_type = config_struct.config_type

    if int(config_type) != 6:
        print("The config provided in not type6 config. Please check.")
        return False

    config_json = json.loads(config_struct.config_json)

    walk_start_date = int(config_json["walk_start_date"])
    trigger_string = config_json["trigger_string"]
    reg_string = config_json["reg_string"]
    ddays_string = config_json["ddays_string"]
    td_string = config_json['td_string']
    rd_string = config_json['rd_string']
    rdata_process_string = config_json['rdata_process_string']

    todays_date = int(time.strftime("%Y%m%d"))  # 20160827, For testing
    analyse_start_date = int(walk_start_date)

    if "model_process_string" in config_json.keys():
        model_process_list = list(map(float,config_json["model_process_string"].split()))
        previous_stdev_present = model_process_list[0]
        if previous_stdev_present != 0 and len(model_process_list) > 1:
            previous_stdev = model_process_list[1]
        else:
            previous_stdev = "NA"
    else:
        previous_stdev = "NA"

    if previous_stdev == "NA":
        print("Stdev was not specified while creating config. Can't rescale")
        return False

    if is_target_stdev:
        target_stdev = stdev
    else:
        target_stdev = previous_stdev
        previous_stdev = stdev

    scale_factor = target_stdev / previous_stdev

    print("\nUpdating model stdev to ",target_stdev, " for config:", config_file)
    print("Fetching model coeffs for config")
    model_refresh_data = fetch_model_present_dates_for_config(config_file, analyse_start_date, todays_date)
    model_refresh_df = pd.DataFrame(model_refresh_data,columns = ['Date','ModelId','Model_Coeffs','ConfigId'])
    model_refresh_df.head()
    model_refresh_dates = model_refresh_df['Date'].tolist()
    model_refresh_dates.sort()

    new_model_coeffs = []
    model_id_list = []
    old_model_coeffs = []
    for i in range(0,len(model_refresh_dates)):
        start_date = int(model_refresh_dates[i])
        model_id = model_refresh_df[model_refresh_df['Date'] == start_date]['ModelId'].tolist()[0]
        model_id_list.append(model_id)
        model_ind_coeffs = model_refresh_df[model_refresh_df['Date']==start_date]['Model_Coeffs'].tolist()[0]
        old_model_coeffs.append(model_ind_coeffs)

        model_ind_coeffs_list = list(map(str, model_ind_coeffs.split(',')))
        new_ind_coeffs = []
        for ind_coeffs in model_ind_coeffs_list:
            if ind_coeffs == '0':
                new_ind_coeffs.append(ind_coeffs)
            else:
                if reg_string.split()[0] == "SIGLR" and len(ind_coeffs.split(":")) > 1:
                    new_ind_coeffs.append(ind_coeffs.split(":")[0] + ":" + str(float(ind_coeffs.split(":")[1])*scale_factor)) # multiplying only beta for scaling the model in case of SIGLR
                else:
                    new_ind_coeffs.append(str(float(ind_coeffs) * scale_factor))

        new_model_coeffs.append(','.join(new_ind_coeffs))

    print("New coeffs found. Now updating them")
    print("\nPrinting configid, modelid, old_coeffs, new_coeffs, date.")

    for i in range(0, len(model_refresh_dates)):
        print(config_id, model_id_list[i], old_model_coeffs[i] ,new_model_coeffs[i], int(model_refresh_dates[i]))
        insert_or_update_model_coeffs(config_id, model_id_list[i], new_model_coeffs[i], int(model_refresh_dates[i]))

    return True

if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument('-c', dest='configname', help="type6 config", type=str, required=True)
    parser.add_argument('-stdev', dest='target_stdev', help="target stdev config", type=float, required=True)

    args = parser.parse_args()

    if args.using_grid and ('GRID_USERNAME' not in os.environ or os.environ['GRID_USERNAME'] == "" or 'GRID_PASSWORD' not in os.environ or os.environ['GRID_PASSWORD'] == ""):
        grid_user = input("Enter username: ")
        if grid_user == "":
            print("Enter valid username")
            sys.exit(1)
        password = getpass.getpass("Enter password: ")
        os.environ['GRID_USERNAME'] = grid_user
        os.environ['GRID_PASSWORD'] = password

    update_type6_model_stdev(args.configname, args.target_stdev)
