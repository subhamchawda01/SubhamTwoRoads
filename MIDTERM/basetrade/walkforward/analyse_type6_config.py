#!/usr/bin/env python
"""
Script to build models based on pnl space 
"""

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
import matplotlib
matplotlib.use('Agg')
from matplotlib import pyplot as plt
sys.path.append(os.path.expanduser('~/basetrade/'))
sys.path.append(os.path.expanduser('~/grid/'))

from walkforward.utils.date_utils import calc_prev_week_day
from walkforward.utils.date_utils import week_days_between_dates
from walkforward.wf_db_utils.fetch_config_details import fetch_config_details
from walkforward.wf_db_utils.fetch_model_present_dates_for_config import fetch_model_present_dates_for_config
from walkforward.utils.prepare_tdata import run_datagen
from walkforward.utils.process_pduration import get_duration_in_algospace
from walkforward.utils.prepare_rdata import convert_t2d
from walkforward.utils.process_rdata import apply_filters
from walkforward.wf_db_utils.utils import FileExistsWithSize
from walkforward.wf_db_utils.fetch_dump_model import fetch_model_desc_from_configid
from walkforward.wf_db_utils.fetch_dump_param import fetch_param_desc_from_configid
from walkforward.utils.dump_content_to_file import write_model_to_file

from grid.client.api import GridClient

from pylib.pnl_modelling_utils.generate_pnl_stats import *
from pylib.pnl_modelling_utils.weights_util import *
from pylib.pnl_modelling_utils.ilist_utils import *
from pylib.pnl_modelling_utils.summarize_results_choose_strat import summarize_results_and_choose
from pylib.pnl_modelling_utils.summarize_results_choose_strat import find_best_strat_on_validation_days

from email.mime.text import MIMEText
from email.mime.image import MIMEImage
from email.mime.multipart import MIMEMultipart

GRID_URL = "http://10.1.4.15:5000"

def get_avg_sample_data(shortcode, feature_list, date_list, start_time, end_time):
    num_days_to_compute_feature = week_days_between_dates(int(min(date_list)), int(max(date_list)))
    counter = 0
    for feature in feature_list:
        avg_feature_val_cmd = [execs.execs().avg_samples, shortcode, str(max(date_list)), \
                               str(num_days_to_compute_feature), start_time, end_time, \
                               '1', feature]
        avg_feature_out = subprocess.Popen(' '.join(avg_feature_val_cmd), shell=True, stdout=subprocess.PIPE)
        x = pd.read_fwf(io.BytesIO(avg_feature_out.communicate()[0]), header=None, widths=[8, 7], \
                        skiprows=1, delimiter=' ')
        x[0] = x[0].astype(int)
        x[1] = x[1].astype(float)
        x.columns = ['Date',feature]
        if counter == 0:
            all_x = x
        else:
            all_x = pd.merge(all_x, x, how='left')
        counter += 1
    return all_x


def get_zero_crossings(s):
    x = s.tolist()
    prev_i = x[0]
    crossings = 0
    for i in x:
        if np.sign(prev_i) != np.sign(i):
            crossings += 1
        prev_i = i
    return crossings

def get_sumvars(indicator_df, model_coeffs, reg_string):

    if reg_string.split()[0] == 'SIGLR':

        simple_flag = True
        for i in range(0,len(model_coeffs)):
            if len(model_coeffs[i].split(':')) > 1:
                simple_flag = False  # In siglr models also, the inital model (on walk start date) is simple, not siglr.
                break

        if simple_flag == True:
            model_coeffs = list(map(float,model_coeffs))
            return indicator_df.dot(model_coeffs)

        indicator_list = indicator_df.columns.tolist()
        siglr_df = pd.DataFrame()
        for i in range(0,len(indicator_list)):
            siglr_string = model_coeffs[i]
            alpha = float(siglr_string.split(':')[0])
            if len(siglr_string.split(':')) > 1:  # some indicators may have zero weight and thus not split by : 
                beta = float(siglr_string.split(':')[1])
            else:
                beta = 0
            siglr_df[indicator_list[i]] = indicator_df[indicator_list[i]].map(lambda x: beta * (1 / (1 + np.exp(-x * alpha)) - 0.5) )

        return siglr_df.sum(axis=1)
    else:
        model_coeffs = list(map(float,model_coeffs))
        return indicator_df.dot(model_coeffs)

def get_relative_weights(indicator_df, model_coeffs, reg_string):

    if reg_string.split()[0] == 'SIGLR':
        return []
    else:
        model_coeffs = list(map(float, model_coeffs))
        indicator_list = indicator_df.columns.tolist()
        stdev_normalized_list = []
        for i in range(0, len(model_coeffs)):
            stdev_normalized_list.append(abs(model_coeffs[i]*np.std(indicator_df[indicator_list[i]])))

        sum = 0
        min_stdev_normalized = np.min(stdev_normalized_list)
        min_stdev_normalized_list = []
        for i in range(0, len(model_coeffs)):
            min_stdev_normalized_list.append(round(stdev_normalized_list[i] / min_stdev_normalized, 2))

        sum_coeffs = np.sum(min_stdev_normalized_list)
        relative_weight_list = []
        for i in range(0, len(model_coeffs)):
            relative_weight_list.append(int(min_stdev_normalized_list[i]/sum_coeffs * 10 * np.sign(model_coeffs[i])))

    return relative_weight_list

def get_model_statistics(model_coeffs, date_list, reg_string, work_dir, need_relative_weights=False):

    if len(date_list) == 0:
        return {'Model_Stdev':0,'Model_Corr':0,'Zero_Crossings':0,'L1_bias_stdev':0}, []

    # When ever we compute model statistics, we append the relevant dates reg_data output into this file. If this exists, remove it.
    temp_catted_file = work_dir + "/temp_catted_file"

    if FileExistsWithSize(temp_catted_file):
        os.system('rm -rf '+ temp_catted_file)

    for date in date_list:
        os.system("cat " + work_dir + "/rdata_file_"+ str(date) + ">> " + temp_catted_file)

    read_df = pd.read_csv(temp_catted_file,sep=' ', header=None)

    y = read_df[0]
    x = read_df[list(range(1,len(read_df.columns.tolist()) -1 ))]
    libias_column = read_df[len(read_df.columns.tolist())-1]

    sumvars = get_sumvars(x, model_coeffs, reg_string)

    if need_relative_weights:
        relative_weight_list = get_relative_weights(x, model_coeffs, reg_string)
    else:
        relative_weight_list = []

    return {'Model_Stdev':np.std(sumvars),'Model_Corr':y.corr(sumvars),'Zero_Crossings':get_zero_crossings(sumvars)/len(date_list),\
            'L1_bias_stdev':np.std(libias_column)}, relative_weight_list

def get_model_statistics_for_date(model_refresh_date, model_coeffs, testing_end_date, lookback_days_for_model_learning, reg_string, all_dates_possible, \
                                  walk_start_date, feature_list, sample_data, work_dir):

    model_coeffs_list = list(map(str, model_coeffs.split(',')))
    all_dates_possible = list(map(int,all_dates_possible))

    least_date_after_start = min([i for i in all_dates_possible if i>=model_refresh_date])
    max_date_before_end = max([i for i in all_dates_possible if i<=testing_end_date])

    gap = 1 if least_date_after_start==model_refresh_date else 0  # to ensure not including model refresh day in summarize results as we refresh at EOD

    start_index = all_dates_possible.index(least_date_after_start)
    end_index = all_dates_possible.index(max_date_before_end)

    if model_refresh_date == walk_start_date:
        training_dates = []
    else:
        training_dates = all_dates_possible[start_index-lookback_days_for_model_learning+1:start_index+1]

    testing_dates = all_dates_possible[start_index + gap:end_index + 1]

    out = {}
    out['Train'], relative_weight_list = get_model_statistics(model_coeffs_list, training_dates,reg_string, work_dir, need_relative_weights=True )
    out['Test'], empty_list = get_model_statistics(model_coeffs_list, testing_dates, reg_string, work_dir)

    data = sample_data[sample_data['Date'].isin(testing_dates)]
    for feature in feature_list:
        out['Test']["Product_" + feature] =  np.mean(data[feature])

    return out, relative_weight_list

def add_l1bias_indicator_to_ilist_file(ilist):

    ilist_lines = open(ilist, 'r').read().splitlines()
    for lines in ilist_lines:
        tokens = lines.strip().split()
        if tokens[0][0] == "#":
            continue
        elif tokens[0] == "MODELINIT":
            shortcode = tokens[2]
            base_price = tokens[3]
            break

    indicator_string = "INDICATOR 1.0 DiffPairPriceType " + shortcode + " MidPrice " + base_price + " # Indicator not in main ilist. Added by code."
    ilist_lines.insert(-1, indicator_string)

    f = open(ilist, 'w')
    for lines in ilist_lines:
        f.write(lines)
        f.write('\n')
    f.close()


def generate_data(shortcode, config_id, dates, num_days, start_time, end_time,
                datagen_args, regdata_args, regdata_process_filter,
                reg_string, parent_work_dir=None, using_grid = True):

    work_dir = parent_work_dir

    catted_file = work_dir + "/catted_rdata_file"
    if FileExistsWithSize(catted_file):
        open(catted_file, 'w').close()

    filtered_file = work_dir + "/filtered_rdata_file"
    if FileExistsWithSize(filtered_file):
        open(filtered_file, 'w').close()

    reg_output_file = work_dir + "/reg_output_file"
    if FileExistsWithSize(reg_output_file):
        open(reg_output_file, 'w').close()

    final_model_file = work_dir + "/final_model_file"
    if FileExistsWithSize(final_model_file):
        open(final_model_file, 'w').close()

    dates_file = work_dir + "/dates_file"
    if FileExistsWithSize(dates_file):
        open(dates_file, 'w').close()

    ilist = work_dir + "/ilist_file"
    if FileExistsWithSize(ilist):
        open(ilist, 'w').close()

    param = work_dir + "/param_file"
    if FileExistsWithSize(param):
        open(param, 'w').close()

    dfh = open(dates_file, 'w')
    for date in dates:
        dfh.write("%s\n" % date)
    dfh.close()

    model_desc = fetch_model_desc_from_configid(config_id, 0)
    param_desc = fetch_param_desc_from_configid(config_id, 0)
    f = open(ilist, 'w')
    f.write(model_desc)
    f.close()

    add_l1bias_indicator_to_ilist_file(ilist)

    f = open(param, 'w')
    f.write(param_desc)
    f.close()

    datagen_args = datagen_args.split()
    ptime = datagen_args[0]
    events = datagen_args[1]
    trades = datagen_args[2]

    regdata_args = regdata_args.split()
    if regdata_args[1] == "VOL" or regdata_args[1] == "STDEV":
        feature = regdata_args[1]
        pred_algo = regdata_args[0]
        duration_in_feature_space = regdata_args[2]
        pred_dur_cmd = [execs.execs().dynamic_pred_dur, shortcode, str(date), str(num_days),
                        start_time, end_time, feature, duration_in_feature_space]
        p_feat = subprocess.Popen(' '.join(pred_dur_cmd), shell=True,
                                  stderr=subprocess.PIPE,
                                  stdout=subprocess.PIPE)
        out, err = p_feat.communicate()
        if out is not None:
            out = out.decode('utf-8')
        if err is not None:
            err = err.decode('utf-8')
        errcode = p_feat.returncode
        if len(err) > 0:
            raise ValueError("Error in retrieving dynamic pred duration")

        duration_in_tspace = float(out)
        #print(out)

    else:
        pred_algo = regdata_args[0]
        duration_in_tspace = regdata_args[1]

    duration_in_algo_space = get_duration_in_algospace(shortcode, duration_in_tspace, date, pred_algo, start_time, end_time)
    #print("Dynamic Predduration ", duration_in_tspace,duration_in_algo_space)

    # Running Datagen with grid
    if using_grid:
        print("Using grid for datagen")
        grid_run_dates=[]
        for t_date in dates:
            #print(t_date)
            rdata_file = work_dir + "/rdata_file_" + t_date
            if not FileExistsWithSize(rdata_file):
                #print(t_date)
                grid_run_dates.append(t_date)

        if len(grid_run_dates) != 0 :
            grid_location_list = []
            grid_indiviual_run_dates = []
            #Grid run takes only 300 days in a single run
            grid_run_required = int(len(grid_run_dates)/300) + 1
            for i in range(0, grid_run_required):
                dates_local = grid_run_dates[i*300:(i+1)*300 - 1]
                grid_indiviual_run_dates.append(dates_local)
                json_file = work_dir + "/datagen_" + str(i) + ".json"
                f = open(json_file,'w')
                json_object = {"job": "generate_data","ilist": ilist,"msecs": ptime,"l1events": events,"trades": trades,\
                               "eco_mode": "0","start_time": start_time, "end_time": end_time,"dates": dates_local,"stats_args": None}
                f.write(json.dumps(json_object))
                f.close()
                grid_client = GridClient(server_url=GRID_URL, username=os.getenv("GRID_USERNAME"),
                                 password=os.getenv("GRID_PASSWORD"), grid_artifacts_dir=work_dir)
                grid_output_location = grid_client.submit_job(json.dumps(json_object))
                grid_location_list.append(grid_output_location + "/artifacts/datagen/")

            #combine grid runs
            for i in range(0, grid_run_required):
                location = grid_location_list[i]
                dates_local =  grid_indiviual_run_dates[i]
                for t_date in dates_local:
                    grid_file_location = location + "/" + t_date + ".txt"
                    destination_location = work_dir + "/tdata_file_" + t_date
                    os.system("mv " + grid_file_location + " " + destination_location)
                os.system("rm -rf " + location)

    # Converting datagen output to regdata
    for t_date in dates:
        #print(t_date)
        tdata_file = work_dir + "/tdata_file_" + t_date
        rdata_file = work_dir + "/rdata_file_" + t_date
        if FileExistsWithSize(rdata_file):
            #print(("rdata file already exists for this date " + t_date))
            continue

        # filter is sent if we have to/can apply it while converting
        # if not using grid, then run datagen
        if not using_grid:
            out, err, errcode = run_datagen(ilist, t_date, start_time, end_time, tdata_file, ptime, events, trades)
            if len(err) > 0:
                print("Error: reported in datagen module", err)
            if errcode != 0:
                print("Error: received non zero exitcode from datagen module", errcode)

        out, err, errcode = convert_t2d(shortcode, t_date, ilist, tdata_file, str(duration_in_algo_space), pred_algo,
                                    rdata_file, regdata_process_filter)

        if len(err) > 0:
            print("Error: reported in t2d module", err)
        if errcode != 0:
            print("Error: received non zero exitcode for, t2d module", errcode)


def generate_analysis_for_config(config_file, mail_address, using_grid, work_dir=None, analyse_start_date=None, analyse_end_date=None):
    '''
    Reads the pnl model config file and calls the best_pnl_model with appropriate arguments

    config_file: str
                The full path of the config file

    using_grid: boolean
                Whether of use grid or not
    '''

    matplotlib.rcParams['font.size'] = 20

    wf_config = os.path.basename(config_file)

    config_struct = fetch_config_details(wf_config)
    config_struct.sanitize()

    config_id = int(config_struct.configid)
    config_type = config_struct.config_type
    shortcode = config_struct.shortcode
    start_time = config_struct.start_time
    end_time = config_struct.end_time

    if int(config_type) != 6:
        print("The config provided in not type6 config. Please check.")
        sys.exit(431)

    if work_dir == None:
        work_dir = "/media/shared/ephemeral16/" + getpass.getuser() + "/type6_analysis_script/" + shortcode + "/" + \
                    str(int(time.time() * 1000)) + "/"

        os.system("rm -rf " + work_dir)
        os.system("mkdir -p " + work_dir)

    with open(work_dir + "/temp_config_file_name", 'w') as f:
        f.write(wf_config)

    print("Work_dir: ",work_dir)

    config_json = json.loads(config_struct.config_json)

    walk_start_date = int(config_json["walk_start_date"])
    trigger_string = config_json["trigger_string"]
    reg_string = config_json["reg_string"]
    ddays_string = config_json["ddays_string"]
    td_string = config_json['td_string']
    rd_string = config_json['rd_string']
    rdata_process_string = config_json['rdata_process_string']

    if analyse_end_date == None:
        todays_date = int(time.strftime("%Y%m%d"))  # 20160827, For testing
    else:
        todays_date = int(analyse_end_date)

    if analyse_start_date == None:
        analyse_start_date = int(walk_start_date)
    else:
        analyse_start_date = int(analyse_start_date)

    #Check for new format of Dday string
    if len(ddays_string.split('-')) > 1:
        print("Dday string is for similar days. Ignoring similar days here and taking simple lookback days logic")
        ddays_string = int(ddays_string.split('-')[3])
    else:
        ddays_string = int(ddays_string)

    if "model_process_string" in config_json.keys():
        model_process_list = list(map(float,config_json["model_process_string"].split()))
        target_stdev_given = model_process_list[0]
        if target_stdev_given != 0:
            target_stdev = model_process_list[1]
        else:
            target_stdev = "NA"
    else:
        target_stdev = "NA"

    mail_body = ""
    mail_body += "<p>ConfigFile: " + wf_config + "<br>"
    mail_body += "ConfigType: " + str(config_type) + "<br>"
    mail_body += "Analyse Start Date: " + str(analyse_start_date) + "<br>"
    mail_body += "Analyse End Date: " + str(todays_date) + "<br></p>"

    mail_body += "<p>WorkDir: " + str(work_dir) + "</p>"
    mail_body += "<p>Model Files Location = " + work_dir + "/model_files/ </p>"

    mail_body += "<p>Shortcode: " + shortcode + "<br>"
    mail_body += "RegressExec: " + reg_string + "<br>"
    mail_body += "Trigger Periodicity: " + trigger_string + "<br>"
    mail_body += "Lookback days : " + str(ddays_string) + "<br>"
    mail_body += "StartTime: " + start_time + "<br>"
    mail_body += "EndTime: " + end_time + "<br>"
    mail_body += "TargetStdev: " + str(target_stdev) + "<br>"
    mail_body += "Walk Start Date: " + str(walk_start_date)  + "<br></p>"

    print("Fetching refresh dates for config")
    model_refresh_data = fetch_model_present_dates_for_config(config_file, analyse_start_date, todays_date)
    model_refresh_df = pd.DataFrame(model_refresh_data,columns = ['Date','ModelId','Model_Coeffs','ConfigId'])
    model_refresh_df.head()
    model_refresh_dates = model_refresh_df['Date'].tolist()
    model_refresh_dates.sort()

    if todays_date == model_refresh_dates[-1]:  # If model refreshed on the end_date itself, then we do not consider that date as no test period is present for it.
        model_refresh_dates = model_refresh_dates[:-1]

    # Adding current date to the list as for the results of latest model, we will need current date for the testing period
    model_refresh_dates_and_today = model_refresh_dates.copy()
    if todays_date not in model_refresh_dates_and_today:
        model_refresh_dates_and_today.append(todays_date)
    total_refresh_dates = len(model_refresh_dates_and_today)

    mail_body += "<p>Number of days model refreshed = " + str(len(model_refresh_dates)) + "</p>"
    mail_body += "<p>Model Refresh Date, Summarize output line for test days... <br>"

    print("Summarizing results for each period")
    pnl_points = []
    strat_volume_dict = {}
    results_avilable_dates = []
    # Computing result of each model for the testing period i.e. till next model is generated.
    # Also computing overall result as the end to get a sense of comparision
    for i in range(0,total_refresh_dates):
        if (i == total_refresh_dates-1):
            start_date = model_refresh_dates_and_today[0]
            end_date = model_refresh_dates_and_today[i]
            results_row_identifier = "Overall"
            mail_body += "<br>"
        else:
            start_date = model_refresh_dates_and_today[i]
            end_date = model_refresh_dates_and_today[i+1]
            results_row_identifier = str(start_date)

        week_days_between_st_end = week_days_between_dates(start_date,end_date)

        summarize_cmd = [execs.execs().summarize_strategy, shortcode,
                         work_dir + "/temp_config_file_name", "DB", str(start_date), str(end_date), "IF", 'kCNAPnlSharpeAverage', "0", "IF", "1"]

        # summarize_output = subprocess.check_output(summarize_cmd).splitlines()
        out = subprocess.Popen(' '.join(summarize_cmd), shell=True, stdout=subprocess.PIPE)
        sum_out, sum_err = out.communicate()
        if sum_out is not None:
            sum_out = sum_out.decode('utf-8')
        if sum_err is not None:
            sum_err = sum_err.decode('utf-8')

        summarize_output = sum_out.strip().splitlines()
        if len(summarize_output) == 1:
            for line in summarize_output:
                tokens = line.split()
                if len(tokens) > 1:
                    if tokens[0] == 'STRATEGYFILEBASE':
                        mail_body +=  results_row_identifier + " " + ' '.join(tokens[2:]) + "<br>"
                        if results_row_identifier != "Overall":
                            results_avilable_dates.append(start_date)
                            pnl_points.append(tokens[2])
                            strat_volume_dict[start_date] = tokens[4]

    mail_body += "</p>"

    # Plot pnl sharpe of each model
    fig = plt.figure(figsize=(23, 23))
    plt.plot([dt.datetime.strptime(str(elem), "%Y%m%d").date() for elem in results_avilable_dates], pnl_points, marker='o')
    plt.title("Test Period Pnl of Model")
    fig.savefig(work_dir + "/pnl_graph.png")


    # This can be the least date. Before this date, no training could have been done
    least_date_possible = calc_prev_week_day(analyse_start_date, ddays_string)

    # Getting dates for datagen
    dates_cmd = [execs.execs().get_dates, shortcode, str(least_date_possible), str(todays_date)]
    process = subprocess.Popen(' '.join(dates_cmd), shell=True,
                               stderr=subprocess.PIPE,
                               stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')
    errcode = process.returncode
    if len(err) > 0:
        raise ValueError("Error in retrieving dates")

    all_dates_possible = out.split()
    all_dates_possible.sort()

    print("Running datagen for all dates")
    generate_data(shortcode, config_id, all_dates_possible, ddays_string, start_time, end_time, \
                  td_string, rd_string, rdata_process_string, reg_string, work_dir, using_grid)

    if todays_date not in all_dates_possible: # It may be possible that today's date is a weekend/holiday so update model_refresh_dates_and_today
        model_refresh_dates_and_today = model_refresh_dates_and_today[:-1]
        if all_dates_possible[-1] not in model_refresh_dates_and_today:
            model_refresh_dates_and_today.append(all_dates_possible[-1])

    print("Fetching average sample features")
    avg_sample_features = ['STDEV','VOL']
    sample_data = get_avg_sample_data(shortcode,avg_sample_features,all_dates_possible,start_time,end_time)

    # Model desc is required for outputting model in each refresh
    model_desc = fetch_model_desc_from_configid(config_id)
    os.system("mkdir -p " + work_dir + "/model_files/")

    # Get model statistics here
    output = {}
    for i in range(0,total_refresh_dates-1):
        start_date = int(model_refresh_dates_and_today[i])
        end_date = int(model_refresh_dates_and_today[i+1])
        print("Getting model details for start date =", start_date)
        model_coeffs = model_refresh_df[model_refresh_df['Date']==start_date]['Model_Coeffs'].tolist()[0]

        # Write model to a file
        model_file_name = work_dir + "/model_files/" + str(start_date) + "_model.txt"
        write_model_to_file(model_file_name, 'NAfile.txt', model_coeffs, reg_string.split()[0] == "SIGLR", model_desc)

        # Get model statistics
        output[start_date], relative_weight_list = get_model_statistics_for_date(start_date, model_coeffs, end_date, ddays_string, reg_string, list(map(int,all_dates_possible)), walk_start_date, \
                                                           avg_sample_features, sample_data, work_dir)

        if len(relative_weight_list) != 0:
            f = open(model_file_name,'a')
            f.write("# Relative Weights: " + " ".join(list(map(str,relative_weight_list))))
            f.close()

    metric_sorter = ['Model_Corr', 'Model_Stdev','Zero_Crossings','L1_bias_stdev'] + ["Product_" + feature for feature in avg_sample_features] + ['Strat_Volume']

    out_df = pd.Panel(output).to_frame().reset_index()
    out_df.rename(columns={'major':'MetricName','minor':'DataSet'},inplace=True)

    out_df.MetricName = out_df.MetricName.astype("category")
    out_df.MetricName.cat.set_categories(metric_sorter,inplace=True)
    out_df.sort_values(by=['DataSet','MetricName'],ascending=[False,True],inplace=True)

    # Convert volume from a 5 min avg to daily values
    if 'VOL' in avg_sample_features:
        hhmm_utc_start = format(int(subprocess.Popen(execs.execs().get_utc_hhmm + " " + start_time, shell=True,
                                                 stdout=subprocess.PIPE).communicate()[0]), '04d')
        hhmm_utc_end = format(int(subprocess.Popen(execs.execs().get_utc_hhmm + " " + end_time, shell=True,
                                               stdout=subprocess.PIPE).communicate()[0]), '04d')
        min_from_midnight_start = 60 * int(hhmm_utc_start[:2]) + int(hhmm_utc_start[2:])
        min_from_midnight_end = 60 * int(hhmm_utc_end[:2]) + int(hhmm_utc_end[2:])
        diff_min = min_from_midnight_end - min_from_midnight_start
        volume_multiplier = (1440 + diff_min) / 5.0 if diff_min < 0 else diff_min / 5.0
        vol = out_df[out_df['MetricName'] == 'Product_VOL'][model_refresh_dates].iloc[0].tolist()
        out_df[out_df['MetricName'] == 'Product_VOL'] = ['Product_VOL', 'Test'] + [i * volume_multiplier for i in vol]

    strat_volume_list = [strat_volume_dict[date] if date in strat_volume_dict.keys() else 0 for date in model_refresh_dates]

    out_df.loc[out_df.shape[0]] = ['Strat_Volume', 'Test'] + strat_volume_list
    #print(out_df)
    out_df.to_csv(work_dir + '/models_statistics')

    summary_temp_df =  out_df[out_df['DataSet']=='Test'].copy()
    summary_temp_df.drop('DataSet',axis=1,inplace=True)
    summary_temp_df = summary_temp_df.T
    summary_temp_df.columns = summary_temp_df.ix["MetricName"].tolist()
    summary_temp_df.drop('MetricName',inplace=True)
    summary_temp_df = summary_temp_df.astype(float)
    summary_df = summary_temp_df.describe()

    print("Generating graphs")
    for metric in ['Model_Corr', 'Model_Stdev', 'Zero_Crossings']:
        for dataset in ['Train','Test']:
            metric_values = out_df[((out_df['MetricName']==metric) & (out_df['DataSet']==dataset))][model_refresh_dates].iloc[0].tolist()
            fig = plt.figure(figsize=(23, 23))
            plt.plot([dt.datetime.strptime(str(elem), "%Y%m%d").date() for elem in model_refresh_dates], metric_values, marker='o')
            plt.title(dataset + " " + metric)
            fig.savefig(work_dir + "/" +dataset + "_" + metric + "_graph.png")
    
    for feature in avg_sample_features:
        metric = "Product_" + feature 
        for dataset in ['Test']:
            metric_values = out_df[((out_df['MetricName']==metric) & (out_df['DataSet']==dataset))][model_refresh_dates].iloc[0].tolist()
            fig = plt.figure(figsize=(23, 23))
            plt.plot([dt.datetime.strptime(str(elem), "%Y%m%d").date() for elem in model_refresh_dates], metric_values, marker='o')
            plt.title(dataset + " " + metric)
            fig.savefig(work_dir + "/" + dataset + "_" + metric + "_graph.png")

    metric_values = out_df[((out_df['MetricName']=="Model_Corr") & (out_df['DataSet']=="Test"))][results_avilable_dates].iloc[0].tolist()
    fig = plt.figure(figsize=(23, 23))
    ax1 = fig.add_subplot(111)
    ax1.set_ylabel('y1')
    ax1.plot([dt.datetime.strptime(str(elem), "%Y%m%d").date() for elem in results_avilable_dates], pnl_points, marker='o',color='black')
    ax1.set_ylabel('PnL')
    ax2 = ax1.twinx()
    ax2.plot([dt.datetime.strptime(str(elem), "%Y%m%d").date() for elem in results_avilable_dates], metric_values, marker='o',color='b',linestyle='--')
    ax2.set_ylabel('Model Corr',color='b')
    for tl in ax2.get_yticklabels():
        tl.set_color('b')
    plt.title("Test Period Pnl with Test Model Corr")
    fig.savefig(work_dir + "/Test_PnL_Model_Corr_combined_graph.png")

    mail_body += "<p> Model Statistics: <br>"
    mail_body += out_df.T.to_html(header=False)
    mail_body += "</p>"

    mail_body += "<p> Model Statistics Summary for Test Set: <br>"
    mail_body += summary_df.to_html(header=True)
    mail_body += "</p>"

    os.system("rm -rf " + work_dir + "/" + "tdata_*")
    os.system("rm -rf " + work_dir + "/" + "rdata_*")

    print("Sending mail")
    #send_email("piyush.vyas@tworoads.co.in", mail_body, work_dir)
    send_email(mail_address, mail_body, work_dir)

def send_email(mail_address, mail_body, work_dir=None):
    msg = MIMEMultipart()
    msg["To"] = mail_address
    msg["From"] = mail_address
    msg["Subject"] = "TYPE6_ANALYSIS " + work_dir
    msg.attach(MIMEText(mail_body, 'html'))

    if work_dir != None:
        for graphfile in glob.glob(work_dir + "/" + "*.png"):
            fp = open(graphfile, 'rb')
            img = MIMEImage(fp.read())
            fp.close()
            img.add_header('Content-Disposition', 'attachment', filename=os.path.basename(graphfile))
            msg.attach(img)

    mail_process = subprocess.Popen(["/usr/sbin/sendmail", "-t", "-oi"],
                                    stdin=subprocess.PIPE,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)
    out, err = mail_process.communicate(str.encode(msg.as_string()))
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')
    errcode = mail_process.returncode

if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument('-c', dest='configname', help="type6 config", type=str, required=True)
    parser.add_argument('--nogrid', dest='using_grid',help='whether to use grid or not',default=True, required=False, action='store_false')
    parser.add_argument('-sd', dest='start_date', help='start date of analysis', default=None, required=False)
    parser.add_argument('-ed', dest='end_date', help='end date of analysis', default=None, required=False)
    parser.add_argument('-work_dir', dest='work_dir', help='work directory (if already present)', default=None, required=False)

    args = parser.parse_args()

    if args.using_grid and ('GRID_USERNAME' not in os.environ or os.environ['GRID_USERNAME'] == "" or 'GRID_PASSWORD' not in os.environ or os.environ['GRID_PASSWORD'] == ""):
        grid_user = input("Enter username: ")
        if grid_user == "":
            print("Enter valid username")
            sys.exit(1)
        password = getpass.getpass("Enter password: ")
        os.environ['GRID_USERNAME'] = grid_user
        os.environ['GRID_PASSWORD'] = password

    generate_analysis_for_config(args.configname, 'nseall@tworoads.co.in', args.using_grid, args.work_dir, args.start_date, args.end_date)
