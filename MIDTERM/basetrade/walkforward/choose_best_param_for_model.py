#!/usr/bin/env python


"""
This module takes input as conifig and tradingdate
Assumption
 a) single model in config
 b) More than 1 param in config

 Input:
 json string should have following things
 a) sort_algo
 b) max_ttc

"""

import os
import json
import random
import shutil
from grid.client.api import GridClient
from scripts.create_json_string import create_json_string
import subprocess
import random
from walkforward.definitions import execs
from walkforward.utils.date_utils import calc_prev_week_day
import shutil
from walkforward.wf_db_utils.get_optimized_days_for_which_model_param_is_missing import \
    get_optimized_days_for_which_model_param_is_missing

from walkforward.wf_db_utils.get_trading_days_for_shortcode import get_trading_days_for_shortcode


def choose_best_param_for_model(cfg, configname, tradingdate, num_days, day_to_model_param_pair, overwrite):
    """
    pick best param out of given params for a model in pnl space

    :param cfg: 
    :param configname: 
    :param tradingdate: 
    :param num_days: 
    :param day_to_model_param_pair: 
    :param overwrite: 
    :return: 
    """
    # print "Call: choose_best_param_for_model: " + cfg.config_json + str(tradingdate)
    config_json = json.loads(cfg.config_json)
    if 'param_lookback_days' in config_json:
        lookback_days = int(config_json['param_lookback_days'])
    else:
        print("Lookback Days not provided, using 5 as default")
        lookback_days = 5

    if 'update_days_interval' in config_json:
        update_days_interval = config_json['update_days_interval']
    else:
        print(("UpdateDays interval not provided, using lookback_days/2 " + str(int(lookback_days) / 2)))
        update_days_interval = int(lookback_days) / 2

    date_pair_vec_to_run_sim = []
    no_results_day_vec = []
    tr_start_date = calc_prev_week_day(tradingdate, int(num_days) + lookback_days)
    if not overwrite:
        (date_pair_vec_to_run_sim, no_results_day_vec) = get_optimized_days_for_which_model_param_is_missing(cfg,
                                                                                                             configname,
                                                                                                             num_days,
                                                                                                             tradingdate,
                                                                                                             lookback_days)
    else:
        no_results_day_vec = get_trading_days_for_shortcode(cfg.shortcode, num_days, tradingdate, lookback_days)
        date_pair_vec_to_run_sim.append((tr_start_date, tradingdate))

    print(no_results_day_vec)
    print(date_pair_vec_to_run_sim)

    modelfilelist = config_json['model_list']
    if len(modelfilelist) > 1:
        print("choose_best_param_for_model: Number of models > 1, "
              "Please check the cfg. Continuing with first one..")

    modelfilename = ''
    for model in modelfilelist:
        if os.path.exists(model):
            modelfilename = model
            break

    new_paramlist = []
    paramfilelist = config_json['param_list']
    for param in paramfilelist:
        if not os.path.exists(param):
            print("choose_best_param_for_model: param: " + param + "does not exist. Skipping")
        else:
            new_paramlist.append(param)

    if len(new_paramlist) == 0:
        print("choose_best_param_for_model: Number of valid params 0. exiting()")
        exit(0)
    elif len(new_paramlist) == 1:
        print("choose_best_param_for_model: Only one param found, returning it")
        return (modelfilename, new_paramlist[0])
    else:
        # shifting one day back
        tradingdate = calc_prev_week_day(tradingdate, 1)
        work_dir = os.path.join(execs.paths().shared_ephemeral_fbpa, str(random.randint(10, 1000000)))
        strats_dir = os.path.join(work_dir, 'strats')
        local_results_base_dir = os.path.join(work_dir, 'local_results_base_dir')
        logfilename = os.path.join(work_dir, 'main_log_file.txt')

        if not os.path.exists(work_dir):
            os.makedirs(work_dir)
            os.makedirs(strats_dir)
            os.makedirs(local_results_base_dir)
        else:
            print("Rare. The working directory already exists")

        logfile = open(logfilename, 'w')

        strategy_index = 0

        for param in new_paramlist:
            strategy_line = 'STRATEGYLINE ' + cfg.shortcode + ' ' + cfg.execlogic + ' ' + modelfilename + ' ' \
                            + param + ' ' + cfg.start_time + ' ' + cfg.end_time + ' ' + str(cfg.query_id)

            if cfg.event_token:
                strategy_line += (' ' + cfg.event_token)

            strategy_path = strats_dir + '/strat_' + str(strategy_index)
            with open(strategy_path, 'w') as strat:
                strat.write(strategy_line + '\n')
                logfile.write('STRAT: ' + strategy_line + '\n')
                strat.close()
                strategy_index += 1

        # start_date = calc_prev_week_day(tradingdate, lookback_days + num_days)
        model_dict = {}
        param_dict = {}
        for pair in date_pair_vec_to_run_sim:
            if len(pair) == 0:
                continue
            start_date = pair[0]
            end_date = pair[1]

            # we would want this to be a blocking call even in case of distributed version
            # run_simulations_cmd = [execs.execs().run_simulations, cfg.shortcode, strats_dir, str(sd), \
            #                        str(ed), local_results_base_dir, '-d', '0']
            # logfile.write("RUN_SIMULATIONS_CMD\n" + ' '.join(run_simulations_cmd))
            #
            # # run_simulations_output = subprocess.check_output(run_simulations_cmd)
            # out = subprocess.Popen(' '.join(run_simulations_cmd), shell=True, stdout=subprocess.PIPE)
            # run_simulations_output = out.communicate()[0].strip()
            #
            # logfile.write("RUN_SIM_OUTPUT\n" + run_simulations_output)
            shortcode = cfg.shortcode
            strats_dir = os.path.join(work_dir, 'strats')

            # the model and param files should be in ephemeral drives for grid to work, so checking whehter they are present or not
            # if not present then adding them to the ephemeral devices

            for index, strat_file in enumerate(os.listdir(strats_dir)):
                strat_file_full_path = os.path.join(work_dir, "strats", strat_file)
                model_file = os.popen("cat " + strat_file_full_path + " " + "|awk '{print $4}'").read()
                # chomping the newline character
                model_file = model_file.rstrip()
                param_file = os.popen("cat " + strat_file_full_path + " " + "|awk '{print $5}'").read()
                # chomping the newline character
                param_file = param_file.rstrip()
                temp_model_param_directory = work_dir + "/" + str(random.randrange(1, 10000))

                # check if the directory exist or not
                if not os.path.exists(temp_model_param_directory):
                    os.makedirs(temp_model_param_directory)
                else:
                    shutil.rmtree(temp_model_param_directory, ignore_errors=True)
                    os.mkdir(temp_model_param_directory)

                new_model_file_path = os.path.join(temp_model_param_directory, "model_" + str(index))
                new_param_file_path = os.path.join(temp_model_param_directory, "param_" + str(index))
                model_dict[str(new_model_file_path)] = model_file
                param_dict[str(new_param_file_path)] = param_file
                # copying the new model and param file to the new location

                os.system("cp " + str(model_file) + " " + new_model_file_path)
                os.system("cp " + str(param_file) + " " + new_param_file_path)

                # replacing the new model and param file in strat_file

                with open(strat_file_full_path)as strat_file_handle:
                    strat_file_content = strat_file_handle.readlines()[0]

                strat_file_content = strat_file_content.split(" ")
                # replacing the model file
                strat_file_content[3] = new_model_file_path
                # replacing the param file
                strat_file_content[4] = new_param_file_path

                # overwriting the new strat file
                with open(strat_file_full_path, 'w') as strat_file_handle:
                    strat_file_handle.write(" ".join(strat_file_content))

            json_string = create_json_string(strats_dir, start_date, end_date)

            client = GridClient(server_url="http://10.1.4.15:5000", username=os.getenv("GRID_USERNAME"),
                                password=os.getenv("GRID_PASSWORD"))
            if "EOD" in os.environ:
                client = GridClient(server_url="http://10.1.4.28:5000",username=os.getenv("GRID_USERNAME"),
                                    password=os.getenv("GRID_PASSWORD"))
            grid_results_directory = client.submit_job(json_string)

            grid_results_directory = grid_results_directory.split('.')[1]
            orignal_dir = os.getcwd()

            grid_results_directory = orignal_dir + "/" + grid_results_directory + "/artifacts/pnls"
            print("GRID RESULTS PATH", grid_results_directory)
            local_results_base_dir = os.path.join(work_dir, 'local_results_base_dir')
            os.system("mkdir -p " + local_results_base_dir)

            for file_name in os.listdir(grid_results_directory):
                # get yr month and date
                year = file_name.split(".")[0][:4]
                month = file_name.split(".")[0][4:6]
                day = file_name.split(".")[0][6:]
                # make directory if not exists
                os.system("mkdir -p " + local_results_base_dir + "/" + shortcode + "/" + year)
                os.system("mkdir -p " + local_results_base_dir + "/" + shortcode + "/" + year + "/" + month)
                os.system("mkdir -p " + local_results_base_dir + "/" + shortcode + "/" + year + "/" + month + "/" + day)
                # copy the results file in the directory
                os.system("cp  " + grid_results_directory + "/" + file_name + " " + local_results_base_dir +
                          "/" + shortcode + "/" + year + "/" + month + "/" + day + "/results_database.txt")

            # commenting it for now
            # run_simulations_output = subprocess.check_output(run_simulations_cmd)
            # out = subprocess.Popen(' '.join(run_simulations_cmd), shell=True, stdout=subprocess.PIPE)
            # run_simulations_output = out.communicate()[0].strip()
            # run_simulations_output = run_simulations_output.decode('utf-8')

        for test_start_date in no_results_day_vec:
            # for all days which we don't have results, try adding the params
            test_prev_start_date = calc_prev_week_day(test_start_date, 1)
            test_end_date = calc_prev_week_day(test_start_date, lookback_days)
            (model, param) = summarize_results_and_cloose(config_json, work_dir, logfile, test_end_date,
                                                          test_prev_start_date)
            if model == "INVALID" or param == "INVALID":
                continue
            model = model_dict[model]
            param = param_dict[param]
            day_to_model_param_pair[test_start_date] = (model, param)

        logfile.close()
        # removing the directory containing model and param
        cleanup(work_dir)


def summarize_results_and_cloose(config_json, work_dir, logfile, start_date, end_date):
    """

    :param config_json: 
    :param work_dir: 
    :param logfile: 
    :param start_date: 
    :param end_date: 
    :return: 
    """
    local_results_base_dir = os.path.join(work_dir, 'local_results_base_dir')
    strats_dir = os.path.join(work_dir, 'strats')

    sort_algo = config_json['sort_algo'] if 'sort_algo' in config_json and config_json['sort_algo'] is not None else 'kCNAPnlSharpeAverage'
    max_ttc = config_json['max_ttc'] if 'max_ttc' in config_json else '100000'
    min_volume = config_json['min_volume'] if 'min_volume' in config_json else '1'
    max_msgs = config_json['max_msgs'] if 'max_msgs' in config_json else '-1'
    num_files_to_choose = config_json['num_files_to_choose'] if 'num_files_to_choose' in config_json else '10000'
    min_num_files_to_choose = config_json['min_num_files_to_choose'] if 'min_num_files_to_choose' in config_json else '10000'

    summarize_cmd = [execs.execs().summarize_local_results_dir_and_choose_by_algo, sort_algo, num_files_to_choose,
                     min_num_files_to_choose, '-1', min_volume, max_ttc, max_msgs, local_results_base_dir,
                     str(start_date), str(end_date)]

    print(("SUMMARIZE_CMD:\n" + ' '.join(summarize_cmd)))
    logfile.write("SUMMARIZE_CMD:\n" + ' '.join(summarize_cmd) + "\n")

    # summarize_output = subprocess.check_output(summarize_cmd).splitlines()
    out = subprocess.Popen(' '.join(summarize_cmd), shell=True, stdout=subprocess.PIPE)
    summarize_output = out.communicate()[0].strip()
    summarize_output = summarize_output.decode('utf-8')
    summarize_output = summarize_output.splitlines()

    # Checking if constraints on param profile is required
    external_param_check = False
    if "volume_range" in config_json:
        if len(config_json["volume_range"]) != 0:
            external_param_check = True
    if "ttc_range" in config_json:
        if len(config_json["ttc_range"]) != 0:
            external_param_check = True

    if len(summarize_output) > 1:
        best_strat_name = ''
        strat_name_available = False
        default_strat = ''
        default_strat_not_found = True
        for line in summarize_output:
            line = line.split()
            if len(line) > 1:
                if line[0] == 'STRATEGYFILEBASE':
                    strat_name_available = False
                    # check for default param if param config exists and default strat is not found
                    if default_strat_not_found and 'param_config' in config_json and len(config_json['param_config']) != 0 and \
                       'default_param' in config_json and len(config_json['default_param']) != 0 :
                        param_index = -1
                        with open(os.path.join(strats_dir, line[1])) as strat:
                            strat_line = strat.readline()
                            strat_words = strat_line.split()
                            if len(strat_words) > 6:
                                param_index = os.path.basename(strat_words[4]).split('_')[1]
                        # Default param is the first entry and thus will have index 0
                        if int(param_index) == 0:
                            default_strat = line[1]
                            default_strat_not_found = False
                            continue
                    best_strat_name = line[1]
                    strat_name_available = True
                    if not external_param_check:
                        break
                if line[0] == 'STATISTICS' and strat_name_available:
                    check_pass = True
                    if "volume_range" in config_json:
                        # out of given bounds
                        if float(line[3]) < float(config_json["volume_range"][0]) or float(line[3]) > float(config_json["volume_range"][1]):
                            check_pass = False
                    if "ttc_range" in config_json:
                        # out of given bounds
                        if float(line[7]) < float(config_json["ttc_range"][0]) or float(line[7]) > float(config_json["ttc_range"][1]):
                            check_pass = False
                    if check_pass:
                        # if check is passed then pick this strat. Break out of loop
                        break
                    else:
                        best_strat_name = ''
                        strat_name_available = False
            else:
                print("INVALIDOUTPUT + " + ' '.join(line))

        if best_strat_name == '' and default_strat != '':
            print("Param with given constraints not found. Using default param. Strat name = ", default_strat)
            logfile.write("Param with given constraints not found. Using default param. Strat name = " + default_strat + "\n")
            best_strat_name = default_strat

        logfile.write("BEST_STRAT: " + best_strat_name + "\n")

        if best_strat_name:
            best_strat_full_path = os.path.join(strats_dir, best_strat_name)
            best_strat_line = ""
            if os.path.exists(best_strat_full_path):
                with open(best_strat_full_path) as strat:
                    best_strat_line = strat.readline()
                    strat_words = best_strat_line.split()
                    if len(strat_words) > 6:
                        return strat_words[3], strat_words[4]
                    else:
                        print("choose_best_param_for_model: invalid_strat line")
                        logfile.write("choose_best_param_for_model: invalid_strat line\n")
            else:
                print(("choose_best_param_for_model: summarize giving weird stratnames. " + best_strat_full_path))
                logfile.write("choose_best_param_for_model: summarize giving weird stratnames. " + best_strat_full_path + "\n")

    else:
        print("Summarize didn't work, returning default modelparam pairs")
        logfile.write("Summarize didn't work, returning default modelparam pairs" + "\n")

    return "INVALID", "INVALID"


def cleanup(dirname):
    if os.path.exists(dirname):
        shutil.rmtree(dirname)
