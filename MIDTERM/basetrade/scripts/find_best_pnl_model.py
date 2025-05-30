#!/usr/bin/env python
"""
Script to build models based on pnl space 
"""

from __future__ import print_function
import pdb
import os
import sys
import time
import getpass
import argparse
import random
import getpass
import shutil

sys.path.append(os.path.expanduser('~/basetrade/'))
sys.path.append(os.path.expanduser('~/grid/'))
from walkforward.utils.date_utils import calc_prev_week_day, calc_iso_date_from_str_min1
from pylib.pnl_modelling_utils.generate_pnl_stats import *
from pylib.pnl_modelling_utils.weights_util import *
from pylib.pnl_modelling_utils.cv_pnlmodelling import create_cv_ilist
from pylib.pnl_modelling_utils.ilist_utils import *
from scripts.create_json_string import create_json_string
from email.mime.text import MIMEText
from email.mime.image import MIMEImage
from email.mime.multipart import MIMEMultipart
from pylib.pnl_modelling_utils.utils import create_strats
from pylib.pnl_modelling_utils.utils import create_regime_trading_params
from pylib.pnl_modelling_utils.run_sim import run_sim
from pylib.pnl_modelling_utils.summarize_results_choose_strat import summarize_results_and_choose
from pylib.pnl_modelling_utils.summarize_results_choose_strat import find_best_strat_on_validation_days
from pylib.pnl_modelling_utils.three_step_optim import three_step_optim
from pylib.pnl_modelling_utils.k_fold_validation import *
from pylib.pnl_modelling_utils.preprocess_ilist import build_model
from walkforward.utils.get_list_of_dates import get_list_of_dates
from walkforward.utils.get_random_dates import get_random_dates
from scripts.get_pnl_series_correlation_with_pool import get_top_five_corr_stats
from scripts.get_negative_dates_for_pool import get_n_negative_dates_of_pool
from scripts.summarize_pool_on_feature import load_date_value_map
from scripts.get_rank_of_strat_in_pool import get_rank_of_strat_in_pool
from PyScripts.generate_dates import get_traindates


def get_best_pnl_model(shortcode, execlogic, ilist, param_file_list, start_date, end_date,
                       start_time, end_time, target_stdev_list, choose_top_strats=10, max_sum=None,
                       user_matrix=None, sort_algo="kCNAPnlSharpeAverage", regime_indicator=None, num_regimes=None,
                       regime_trading_indicator_list=None, regimes_to_trade=None,
                       three_step=0, dates_for_sim=None, training_dates_list=None, validation_dates_list=None,
                       test_dates_list=None, using_grid=True,
                       cv_list=None, cv_combinations=None, num_folds=None,
                       max_val_three_step=4,
                       num_weights_three_step=500, cname=None, max_ttc_threshold=None, min_vol_threshold=None):
    '''

    Runs the best pnl model


    shortcode: str
                The product shortcode whose pnl model is to be run

    execlogic: str
                Trading logic to use in the strat file

    ilist:   str
                The full path of the ilist file


    param_file_list: str
                The list of param file

    start_date: str
                Start Date

    end_date: str      
                End date

    start_time: str
                 the strategy start time

    end_time: str
                 the strategy end time

    target_stdev_list: list
                 the list of model target stdev

    choose_top_strats: str
                 the number of top strats to choose
    max_sum:  int 
                 the max sum of all weights in the model

    user_matrix: 
                 user specified weight grid

    sort_algo: str
                sort algo to use when selecting best strategy

    regime_indicator: 
    num_regimes: 

    three_step: int 
                flag to control use of three step optim

    dates_for_sim: all the days to run simulations



    :returns 
    work_dir: str
                The full path of the work directory

    mail_content: str
                The mail string to be sent

    best_model: str
                The full path of the best model 

    best_param:str 
                The full path of the best param 

    '''

    num_strats = int(choose_top_strats)
    mail_content = ""

    work_dir = "/media/shared/ephemeral16/" + getpass.getuser() + "/pnl_modelling/" + shortcode + "/" + \
        str(int(time.time() * 1000)) + "/"

    os.system("rm -rf " + work_dir)
    os.system("mkdir -p " + work_dir)
    training_days_file = work_dir + "training_days_file"

    with open(training_days_file, "w") as tf:
        for dt in training_dates_list:
            tf.write("%s\n" % dt)

    validation_days_file = work_dir + "validation_days_file"
    with open(validation_days_file, "w") as vf:
        for dt in validation_dates_list:
            vf.write("%s\n" % dt)

    testing_days_file = work_dir + "testing_days_file"
    with open(testing_days_file, "w") as tstf:
        for dt in test_dates_list:
            tstf.write("%s\n" % dt)

    training_start_date = str(training_dates_list[0])
    training_end_date = str(training_dates_list[-1])

    test_start_date = str(test_dates_list[0])
    test_end_date = str(test_dates_list[-1])

    val_start_date = str(validation_dates_list[0])
    val_end_date = str(validation_dates_list[-1])

    print("Training Dates File " + training_days_file)
    print("Validation Dates File " + validation_days_file)
    print("Testing Dates File " + testing_days_file)

    mail_content += "<p>Training period between " + str(training_start_date) + " to " + str(training_end_date) + "<br>"
    mail_content += "Validation period between " + str(val_start_date) + " to " + str(val_end_date) + "</p><br>"

    if regime_indicator is not None and num_regimes is not None:
        num_regimes = int(num_regimes)
        new_ilist = os.path.join(work_dir, os.path.basename(ilist))
        create_regime_ilist(regime_indicator, num_regimes, ilist, new_ilist)
        ilist = new_ilist

    if regime_trading_indicator_list and regimes_to_trade:
        temp_param_file_list = []
        create_regime_trading_params(work_dir, regime_trading_indicator_list,
                                     regimes_to_trade, param_file_list, temp_param_file_list)
        param_file_list.extend(temp_param_file_list)
        print(param_file_list)

    # if cv is mentioned, creating new set of indicators,
    if cv_list is not None:
        cv_ind_percentile_combination = []
        if len(cv_combinations) > 0:
            for line in cv_combinations:
                cv = line.split(":")[0].strip()
                indicators = line.split(":")[1].strip().split()
                percentiles = line.split(":")[2].strip().split(",")
                for ind in indicators:
                    for percentile in percentiles:
                        cv_ind_percentile_combination.append(
                            (int(cv), int(ind), (float(percentile.strip().split()[0]), float(percentile.strip().split()[1]))))

        new_ilist = os.path.join(work_dir, os.path.basename(ilist))
        create_cv_ilist(cv_list, cv_ind_percentile_combination, ilist, new_ilist, work_dir,
                        shortcode, start_time, end_time, training_end_date, len(training_dates_list))
        ilist = new_ilist

    # generating data to find the standard deviations and covariance matrix of data
    logfilename = generate_data(shortcode, ilist,
                                start_time, end_time, work_dir, training_dates_list)

    logfile = open(logfilename, 'a')
    logfile.write("TARGET STDEV : " + " ".join(str(target_stdev) for target_stdev in target_stdev_list) + "\n")
    logfile.write("Training Period between: " + str(training_start_date) + " - " + str(training_end_date) + "\n")
    logfile.write("Validation Period between: " + str(val_start_date) + " - " + str(val_end_date) + "\n")
    logfile.flush()
    logfile.close()

    # data = np.loadtxt(work_dir + "filtered_rdata_file")
    pnl_stats_file = os.path.join(work_dir, "IndicatorStats/pnl_stats")

    stdev_indicators, cov = get_covariance_matrix(pnl_stats_file, logfilename)
    if len(stdev_indicators) == 0 or len(cov) == 0:
        print("Datagen exited.")
        sys.exit(1)
    print("Stdev of Indicators: ", stdev_indicators)
    print("Covariance Matrix: ")
    print(cov)

    for stdev in stdev_indicators:
        if stdev <= 0.0000000001:
            print("At least one indicator has zero stdev\n")
            mail_content += "<p>At least one indicator has zero stdev" + "</p><br>"
            sys.exit(1)

    num_indicators = cov.shape[0]
    target_stdev_list = list(map(float, target_stdev_list))

    if three_step == 1:
        print("Starting three step optimization")

        mail_content += "<p>USING THREE STEP OPTIM" + "</p>"
        weights = three_step_optim(ilist, num_indicators, logfilename,
                                   max_val=max_val_three_step, num_weights=num_weights_three_step)
    elif regime_indicator is not None and num_regimes is not None:
        weights = three_step_optim(ilist, num_indicators, logfilename,
                                   max_val=max_val_three_step, num_weights=num_weights_three_step)
    elif cv_list is not None:
        weights = three_step_optim(ilist, num_indicators, logfilename,
                                   max_val=max_val_three_step, num_weights=num_weights_three_step)

    else:

        if user_matrix is not None:
            mail_content += "<p>USING USER SPECIFIED WEIGHTS COMBINATIONS" + "</p>"
            # generating all possible weights for different indicators as specified in user matrix
            weights = generate_weight_grid_from_matrix(0, num_indicators, user_matrix)
            # user matrix provided may create a combination where all indicators have weight of 0. removing such cases
            weights = remove_all_zero_combination(weights)
        elif max_sum is not None:
            mail_content += "<p>USING MAX SUM (" + str(max_sum) + ") CONFIGURATION FOR WEIGHT GENERATION" + "</p>"
            # generating all possible weights for different indicators
            weights = generate_weight_grid(0, num_indicators, int(max_sum))

            # pruning weights based on constraints
            weights = check_constraints_on_weights(weights, int(max_sum))
            weights = remove_similar_distance_weights(weights)
            weights = np.array(weights)
            enforce_sign_check(ilist, weights)

    # scaling weights to target stdev
    scaled_weights = rescale_weights(weights, target_stdev_list, stdev_indicators, cov, logfilename)
    # replicating weights and stdev list based on each other as now a unique weight will be combination of stdev and relative weight
    weights, target_stdev_replicated_list = replicate_weights_for_stdev_list(weights, target_stdev_list)
    # getting a mapping of model weights with stdev to print in the model file
    scaled_wts_stdev_dict = get_scaled_wts_stdev_dict(scaled_weights, target_stdev_replicated_list)
    # removing duplicates in weights grid
    weights, scaled_weights = remove_duplicate_weights(weights, scaled_weights)
    print("Total weights: ", weights.shape[0])

    # creating temporary strats in work_dir to run simulations
    create_strats(work_dir, weights, scaled_weights, scaled_wts_stdev_dict, shortcode, execlogic,
                  start_time, end_time, param_file_list, ilist, logfilename)

    # run simulations on all days
    run_sim(work_dir, shortcode, dates_for_sim, logfilename, using_grid)

    time.sleep(120)

    # getting the final strategy depending on the kind of validation
    # we have two choices, one is either the kfold validation, which returns the most consistent strat in the training
    # period based on the rank. Second option is finding top k strats from training period, and returning the best from
    # them in the validation period
    if num_folds is not None:
        top_strats, _, _, fold_mail_content = kfold_validation_strat(work_dir, shortcode, start_date,
                                                                     end_date, logfilename, sort_algo,
                                                                     num_strats, num_folds=num_folds, to_print=True)
        mail_content += fold_mail_content
        val_mail_content, chosen_strat, best_model, best_param = find_best_strat_on_kfold(work_dir, shortcode, start_date,
                                                                                          end_date, logfilename, sort_algo,
                                                                                          top_strats, weights, ilist, training_days_file,
                                                                                          validation_days_file, testing_days_file,
                                                                                          using_grid)

    elif training_days_file is not None and validation_days_file is not None:

        val_mail_content, chosen_strat, best_model, best_param = find_best_strat_on_validation_days(work_dir, shortcode,
                                                                                                    start_date,
                                                                                                    end_date, logfilename, sort_algo, num_strats,
                                                                                                    weights, ilist, training_days_file,
                                                                                                    validation_days_file, testing_days_file,
                                                                                                    using_grid)

    else:
        print("Either of Training Days File : " + training_days_file +
              " or Validation Days File : " + validation_days_file + " is None\n")
        print("Something went wrong, training and validation days file should be present\n")

    mail_content += val_mail_content
    pool_timings = start_time + "-" + end_time
    strats_dir = os.path.join(work_dir, "strats_dir")
    results_dir = work_dir + "local_results_base_dir"

    max_corr = 1
    min_max_lines = None
    print("getting_strategy rank")
    strategy_rank, num_pool_strats = get_rank_of_strat_in_pool(shortcode, pool_timings, start_date, end_date, "N",
                                                  chosen_strat, strats_dir, results_dir, testing_days_file,
                                                  "kCNAPnlSharpeAverage")

    mail_content += "<p>Strategies rank in the pool during test period : " + str(strategy_rank) + "({})".format(num_pool_strats)+"</p>"
    print("getting max_corr")
    if (num_pool_strats > 0):
        min_max_lines, max_corr = get_top_five_corr_stats(shortcode, pool_timings, start_date, end_date,
                                                "N", os.path.basename(chosen_strat), results_dir)
        mail_content += "<p>Stats with top five configs in the pool" + "</p>"

        mail_content += min_max_lines.to_html(index=False)

    logfile = open(logfilename, 'a')
    logfile.write("\nProcess End Time : " + str(datetime.datetime.now()) + "\n")
    logfile.close()

    ## Code to move strat to stage
    print("Checking if strategy could be moved to staged pool.")
    highly_correlated = int(max_corr > 0.8)

    ## find_absolute_sharpe_value_on_all_three_sets
    sel_strat_dir = os.path.join(work_dir, "selected_strat")
    _, strat_lines, _, _ = summarize_results_and_choose(shortcode, work_dir, str(start_date), str(end_date), 1, logfilename, strat_list=sel_strat_dir, sort_algo=sort_algo, dates_file=os.path.join(work_dir, "run_sim_dates"))
    sel_strat_sharpe_sim = float(strat_lines.splitlines()[0].strip().split()[5])
    sel_strat_avg_vol = int(strat_lines.splitlines()[0].strip().split()[4])
    sel_strat_avg_ttc = int(strat_lines.splitlines()[0].strip().split()[8])

    _, strat_lines, _, _ = summarize_results_and_choose(shortcode, work_dir, str(start_date), str(end_date), 1, logfilename, strat_list=sel_strat_dir, sort_algo=sort_algo, dates_file=testing_days_file)
    sel_strat_sharpe_test = float(strat_lines.splitlines()[0].strip().split()[5])

    strat_cmp_str = "Average sharpe for selected strat on test and sim: {},{}, is_highly_correlated: {}, Number of strats in pool: {}, rank in pool: {}".format(sel_strat_sharpe_test,
                                                                                                 sel_strat_sharpe_sim, highly_correlated, num_pool_strats, strategy_rank) 
    logfile = open(logfilename, "a")
    logfile.write("\n"+strat_cmp_str+"\n")
    #mail_content += "<p>" + strat_cmp_str + "</p>"
    print(strat_cmp_str)
    print("config with cname ", cname)
    sel_strat_criteria = sel_strat_sharpe_test > 0.05 and sel_strat_sharpe_sim > 0.05
    if max_ttc_threshold is not None:
        sel_strat_criteria = sel_strat_criteria and sel_strat_avg_ttc < max_ttc_threshold
        print("max_ttc_threshold not satisfied, will not move this strategy")
    if min_vol_threshold is not None:
        sel_strat_criteria = sel_strat_criteria  and sel_strat_avg_vol > min_vol_threshold
        print("min_vol_threshold not satisfied, will not move this strategy.")
    if (sel_strat_criteria):
        if (num_pool_strats == 0 or ((not highly_correlated) and (((1.0*strategy_rank <= (num_pool_strats/2.0)+1) or num_pool_strats <= 20) and strategy_rank > 0))):
            print("Moving selected strategy to staged pool.")
            mail_content += "<p>Moving selected strategy to staged pool</p>"
            mail_content += "<p>cname in wf_configs table: {}</p>".format(cname)
            logfile.write("\nMoving selected strategy into staged pool.\n")
            sel_strat_file = os.path.join(sel_strat_dir, os.listdir(sel_strat_dir)[0])
            create_config_cmd = [execs.execs().create_config_from_ifile, "-lstrat", sel_strat_file, "--pnl_modelling_strat", "1", "--cname", cname, "-r 1 -g 1 -s 1"]
            out = subprocess.Popen(' '.join(create_config_cmd), shell=True, stdout=subprocess.PIPE)
            stout, sterr = out.communicate()
        else:
            print("Selected Strategy is either highly correlated or not better than median strategy. Not moving to stage.")
            mail_content += "<p> Selected Strategy is either highly correlated or not better than median strategy. Not moving to stage.</p>"
    else:
        print("Selected strategy sharpe values not good enough or min_vol_threshold/max_ttc_threshold not satisfied. Not moving to stage.")
        mail_content += "<p> Selected strategy sharpe values not good enough or min_vol_threshold/max_ttc_threshold not satisfied. Not moving to stage.</p>"
    logfile.close()

    return work_dir, mail_content, best_model, best_param


def send_email(mail_address, mail_body, work_dir):
    msg = MIMEMultipart()
    msg["To"] = mail_address
    msg["From"] = mail_address
    msg["Subject"] = "PNL_MODELLING " + work_dir
    msg.attach(MIMEText(mail_body, 'html'))
    fp = open(work_dir + "/graph.png", 'rb')
    img = MIMEImage(fp.read())
    fp.close()
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


def read_params_from_config(config_file, using_grid):
    '''
    Reads the pnl model config file and calls the best_pnl_model with appropriate arguments

    config_file: str
                The full path of the config file

    using_grid: boolean
                Whether of use grid or not
    '''
    user_matrix = []
    ilist_file_list = []
    param_file_list = []
    regime_trading_indicator_list = []
    regimes_to_trade = []
    target_stdev_list = []
    cv_list = []
    cv_combinations = []
    list_of_strats = []
    training_testing_file_list = []
    current_instruction = None
    shortcode = execlogic = ilist = param_file = \
        start_time = end_time = target_stdev = end_date = num_days = \
        choose_top_strats = max_sum = sort_algo = \
        regime_indicator = num_regimes = mail_address = three_step = skip_days_file = use_days_file = \
        min_vol_threshold = max_ttc_threshold = None
    choose_top_strats = 10
    num_folds = None
    training_days_file = None
    testing_days_file = None
    skip_dates = []
    regress_exec_list = []
    datagen_args_list = []
    regdata_args_list = []
    regdata_process_filter_list = []
    dates_for_preprocessing = []
    num_days_for_preprocessing = 50
    sign_check_string = "0"
    max_val_three_step = 4
    num_weights_three_step = 500
    num_diversification_days = 0
    to_get_feature_dates = False
    pick_regime_to_trade = False
    pick_target_stdev = False
    random_days = False
    use_feature_for_preprocessing = 0
    try:
        with open(config_file, 'r') as cfile:
            for line in cfile:

                if not line.strip():
                    current_instruction = None
                    continue

                line = line.strip()

                if line[0] == '#':
                    continue

                if current_instruction is None:
                    current_instruction = line
                else:
                    if current_instruction == "SHORTCODE":
                        shortcode = line
                    if current_instruction == "EXEC_LOGIC":
                        execlogic = line
                    if current_instruction == "INDICATOR_LIST_FILENAME":
                        ilist_file_list.append(line.strip())
                    if current_instruction == "PARAM_FILENAME":
                        param_file_list.append(list(map(str, line.split())))
                    if current_instruction == "START_TIME":
                        start_time = line
                    if current_instruction == "END_TIME":
                        end_time = line
                    if current_instruction == "TARGET_STDEV":
                        target_stdev_list = line.split()
                    if current_instruction == "PICK_TARGET_STDEV":
                        pick_target_stdev = int(line.strip()) > 0
                    if current_instruction == "END_DATE":
                        end_date = line
                    if current_instruction == "NUM_DAYS":
                        num_days = line
                    if current_instruction == "CHOOSE_TOP_STRATS":
                        choose_top_strats = line
                    if current_instruction == "MAX_SUM_INDICATORS":
                        max_sum = line
                    if current_instruction == "REGIME_INDICATOR":
                        regime_indicator = line
                    if current_instruction == "NUM_REGIMES":
                        num_regimes = line
                    if current_instruction == "MODEL_WEIGHTS":
                        user_matrix.append(list(map(int, line.split())))
                    if current_instruction == "SORT_ALGO":
                        sort_algo = line
                    if current_instruction == "MAIL_ADDRESS":
                        mail_address = line
                    if current_instruction == "THREE_STEP_OPTIM":
                        three_step = int(line)
                    if current_instruction == "SKIP_DAYS":
                        skip_days_file = line
                    if current_instruction == "CONDITIONAL_VARIABLE":
                        cv_list.append(line)
                    if current_instruction == "CV_COMBINATIONS":
                        cv_combinations.append(line)
                    if current_instruction == "USE_DAYS":
                        use_days_file = line
                    if current_instruction == "FOLD":
                        num_folds = int(line)
                    if current_instruction == "TRAINING_TESTING_DAYS_DATE_FILE":
                        training_days_file = line.split(" ")[0]
                        testing_days_file = line.split(" ")[1]

                    if current_instruction == "REGIME_TRADE_INDICATOR":
                        regime_trading_indicator_list.append(list(map(str, line.split())))
                    if current_instruction == "REGIMES_TO_TRADE":
                        regimes_to_trade.append(list(map(str, line.split())))
                    if current_instruction == "PICK_REGIME_TO_TRADE":
                        pick_regime_to_trade = int(line) > 0
                    if current_instruction == "REG_STRING":
                        regress_exec_list.append(line)
                    if current_instruction == "NUM_DAYS_FOR_PREPROCESSING":
                        num_days_for_preprocessing = int(line)
                    if current_instruction == "TD_STRING":
                        datagen_args_list.append(line)
                    if current_instruction == "RD_STRING":
                        regdata_args_list.append(line)
                    if current_instruction == "RDATA_PROCESS_STRING":
                        regdata_process_filter_list.append(line)
                    if current_instruction == "SIGN_CHECK":
                        sign_check_string = line
                    if current_instruction == "MAX_VAL_THREE_STEP":
                        max_val_three_step = int(line)
                    if current_instruction == "NUM_WEIGHTS_THREE_STEP":
                        num_weights_three_step = int(line)
                    if current_instruction == "LIST_OF_STRATS":
                        if line == "ALL":
                            list_of_strats = line
                        else:
                            list_of_strats.append(line)
                    if current_instruction == "NUM_DIVERSIFICATION_DAYS":
                        num_diversification_days = int(line)
                    if current_instruction == "FEATURE":
                        feature = line
                        to_get_feature_dates = True
                    if current_instruction == "FEATURE_SPLIT_PERCENTILE":
                        feature_split_percentile = float(line)
                    if current_instruction == "FEATURE_SPLIT_TYPE":
                        feature_split_type = line
                    if current_instruction == "RANDOM_SPLIT":
                        train_percent = float(line)
                        test_percent = (1 - train_percent) / 2
                        train_percent = (train_percent / (1 - test_percent))
                        random_days = True
                    if current_instruction == "USE_FEATURE_FOR_PREPROCESSING":
                        use_feature_for_preprocessing = int(line)

                    if current_instruction == "MIN_VOLUME_THRESHOLD":
                        min_vol_threshold = int(line.strip())
                    if current_instruction == "MAX_TTC_THRESHOLD":
                        max_ttc_threshold =  int(line.strip())
    except:
        print(config_file + " not readable")
        raise ValueError(config_file + " not readable.")

    if shortcode is None:
        raise ValueError("SHORTCODE is required")
    if execlogic is None:
        raise ValueError("EXEC_LOGIC is required")
    if len(ilist_file_list) == 0:
        raise ValueError("INDICATOR_LIST_FILENAME is required")
    if len(param_file_list) == 0:
        raise ValueError("PARAM_FILENAME is required")
    if start_time is None:
        raise ValueError("START_TIME is required")
    if end_time is None:
        raise ValueError("END_TIME is required")
    if len(target_stdev_list) == 0:
        raise ValueError("TARGET_STDEV is required")
    if end_date is None:
        raise ValueError("END_DATE is required")
    if num_days is None:
        raise ValueError("NUM_DAYS is required")
    if max_sum is None and len(user_matrix) == 0:
        raise ValueError("Either of MAX_SUM or MODEL_WEIGHTS is required")
    if regime_indicator is not None and num_regimes is None:
        raise ValueError("REGIME_INDICATOR specified but NUM_REGIMES not specified")
    if regime_indicator is None and num_regimes is not None:
        raise ValueError("NUM_REGIMES specified but REGIME_INDICATOR not specified")
    if len(cv_list) > 0 and len(cv_combinations) == 0:
        raise ValueError("CONDITIONAL_VARIABLE specified but CV_COMBINATIONS not specified")
    if len(regime_trading_indicator_list) != len(regimes_to_trade):
        raise ValueError("Regime Trade Indicator not equal to regimes to trade")

    param_file_list = [elem[0] for elem in param_file_list]

    rand_indx = random.randint(0, len(ilist_file_list) - 1)
    ilist = ilist_file_list[rand_indx]

    if len(user_matrix) == 0:
        user_matrix = None
    if max_sum is None:
        max_sum = None
    if sort_algo is None:
        sort_algo = "kCNAPnlSharpeAverage"
    if regime_indicator is None and num_regimes is None:
        regime_indicator = None
        num_regimes = None
    if mail_address is None:
        mail_address = "nseall@tworoads.co.in"
    if three_step is None:
        three_step = 0
    elif three_step != 0:
        three_step = 1
    if len(cv_list) == 0:
        cv_list = None
    if len(cv_combinations) == 0:
        cv_combinations = None

    if pick_regime_to_trade:
        rand_indx = random.randint(0, len(regimes_to_trade)-1)
        regime_trading_indicator_list = [regime_trading_indicator_list[rand_indx]]
        regimes_to_trade = [regimes_to_trade[rand_indx]]

    if pick_target_stdev:
        rand_indx = random.randint(0, len(target_stdev_list)-1)
        target_stdev_list = [target_stdev_list[rand_indx]]

    if skip_days_file is not None and skip_days_file != "":
        skip_dates = open(skip_days_file).read().splitlines()

    cname = "w"
    print(execlogic)
    if execlogic == "PriceBasedAggressiveTrading":
        cname += "_pbt"
    elif execlogic == "DirectionalAggressiveTrading":
        cname += "_dat"
    elif execlogic == "PriceBasedVolTrading":
        cname += "_pvol"
    else:
        pass
    cname += "_" + os.path.basename(ilist)
    ## Following adds support for mentioning end_date in format: TODAY-N
    end_date = str(calc_iso_date_from_str_min1(end_date))
    # available_dates = get_list_of_dates(shortcode, num_days, end_date)
    start_date = str(calc_prev_week_day(end_date, lookback_days=int(num_days)))
    available_dates = list(map(int, get_traindates(min_date=start_date, max_date=end_date)))
    available_dates = [dt for dt in available_dates if dt not in skip_dates]
    available_dates.sort()
    #start_date = available_dates[0]

    if random_days:
        test_days = int(test_percent * len(available_dates))
        random_indices = random.sample(range(len(available_dates)), test_days)
        test_dates_list = [available_dates[i] for i in random_indices]
    else:
        test_days = int(0.2 * len(available_dates))
        test_dates_list = available_dates[-test_days:]

    test_dates_list.sort()
    available_dates_for_training = [dt for dt in available_dates if dt not in test_dates_list]
    available_dates_for_training.sort()
    end_date_for_trainining = available_dates_for_training[-1]

    # splitting on validation days, finding the start and end training dates and validations dates
    if training_days_file is not None and testing_days_file is not None:
        # reading the training dates in a list
        with open(training_days_file) as training_days_date_file_handle:
            training_dates_list = training_days_date_file_handle.read().splitlines()
            training_dates_list = [dt for dt in training_dates_list if dt not in skip_dates and dt in available_dates_for_training]

        # reading the testing dates file in a list
        with open(testing_days_file) as testing_days_date_file_handle:
            validation_dates_list = testing_days_date_file_handle.read().splitlines()
            validation_dates_list = [dt for dt in validation_dates_list if dt not in skip_dates and dt in available_dates_for_training]

        dates_for_sim = training_dates_list + validation_dates_list

        if len(regress_exec_list) != 0:
            random_indices = random.sample(range(len(training_dates_list)), min(len(training_dates_list),
                                                                                 num_days_for_preprocessing))
            dates_for_preprocessing = [training_dates_list[i] for i in random_indices]
    else:
        if use_days_file is not None and use_days_file != "":
            with open(use_days_file) as f:
                dates_for_sim = f.read().splitlines()
                dates_for_sim = [dt for dt in dates_for_sim if dt in available_dates_for_training]
        elif list_of_strats != []:
            pool_timings = start_time + "-" + end_time
            negative_dates = get_n_negative_dates_of_pool(shortcode, pool_timings, start_date, end_date, list_of_strats,
                                                          num_diversification_days, available_dates_for_training)
            if use_feature_for_preprocessing:
                dates_for_preprocessing = negative_dates
                dates_for_sim = available_dates_for_training
            else:
                dates_for_sim = negative_dates
        elif to_get_feature_dates:
            date_feauture_value_map = {}
            load_date_value_map(shortcode, start_time, end_time, start_date, end_date,
                                feature, date_feauture_value_map, available_dates_for_training)

            features = np.array([float(val) for val in date_feauture_value_map.values()])
            split_value = np.percentile(features, feature_split_percentile)
            print("Split Value for the feature is : ", split_value)
            feature_split_dates = []
            if feature_split_type == "HIGH":
                for dt in available_dates:
                    if dt in date_feauture_value_map:
                        if (float(date_feauture_value_map[dt])) >= (split_value):
                            feature_split_dates.append(dt)
            elif feature_split_type == "LOW":
                for dt in available_dates:
                    if dt in date_feauture_value_map:
                        if float(date_feauture_value_map[dt]) <= split_value:
                            feature_split_dates.append(dt)
            if use_feature_for_preprocessing:
                dates_for_preprocessing = feature_split_dates
                dates_for_sim = available_dates_for_training
            else:
                dates_for_sim = feature_split_dates

        else:
            dates_for_sim = available_dates_for_training

    work_dir_preprocess = ""
    if len(regress_exec_list) != 0:
        if dates_for_preprocessing == []:
            num_random_days_to_choose = min(num_days_for_preprocessing, len(dates_for_sim))
            random_indices = random.sample(range(len(dates_for_sim)), num_random_days_to_choose)

            dates_for_preprocessing = [dates_for_sim[i] for i in random_indices]
        elif len(dates_for_preprocessing) > num_days_for_preprocessing:
            random_indices = random.sample(range(len(dates_for_preprocessing)), num_days_for_preprocessing)
            dates_for_preprocessing = [dates_for_preprocessing[i] for i in random_indices]

        reg_string = regress_exec_list[random.randint(0, len(regress_exec_list) - 1)]
        datagen_args = datagen_args_list[random.randint(0, len(datagen_args_list) - 1)]
        regdata_args = regdata_args_list[random.randint(0, len(regdata_args_list) - 1)]
        regdata_process_filter = regdata_process_filter_list[random.randint(0, len(regdata_process_filter_list) - 1)]

        work_dir_preprocess, ilist = build_model(shortcode, ilist, dates_for_preprocessing, start_time, end_time,
                                                 datagen_args, regdata_args, regdata_process_filter,
                                                 reg_string, sign_check_string, parent_work_dir=None)

        preprocessing_days_file = work_dir_preprocess + "preprocessing_days_file"
        pf = open(preprocessing_days_file, "w")
        for date in dates_for_preprocessing:
            pf.write("%s\n" % date)
        pf.close()
        print("preprocessing Days File is " + preprocessing_days_file)
        print("\nIlist after preprocessing is " + ilist + "\n")
        cname += "_" + "_".join(regdata_args.split())
        cname += "_" + start_time
        cname += "_" + end_time
        cname += "_" + "_".join(datagen_args.split())
        cname += "_" + regdata_process_filter
        cname += "_" + "_".join(reg_string.split())
        cname += "_" + str(random.randint(0, 10000))
    else:
        cname += "_" + start_time
        cname += "_" + end_time
        cname += "_" + str(random.randint(0, 10000))

    dates_for_sim.sort()

    if training_days_file is None or testing_days_file is None:
        if random_days:
            training_days = int(train_percent * len(dates_for_sim))
            random_indices = random.sample(range(len(dates_for_sim)), training_days)
            training_dates_list = [dates_for_sim[i] for i in random_indices]
            validation_dates_list = [dt for dt in dates_for_sim if dt not in training_dates_list]
        else:
            training_days = int(0.75 * len(dates_for_sim))
            training_dates_list = dates_for_sim[:training_days]
            validation_dates_list = [dt for dt in dates_for_sim if dt not in training_dates_list]
    training_dates_list.sort()
    validation_dates_list.sort()

    mail_body = ""
    mail_body += "<p>ConfigFile: " + os.path.abspath(config_file) + "<br>"
    mail_body += "<p>Shortcode: " + shortcode + "<br>"
    mail_body += "ExecLogic: " + execlogic + "<br>"
    mail_body += "Ilist: " + ilist + "<br>"
    mail_body += "ParamFile: " + " ".join(str(elem) for elem in param_file_list) + "<br>"
    mail_body += "StartTime: " + start_time + "<br>"
    mail_body += "EndTime: " + end_time + "<br>"
    mail_body += "TargetStdev: " + " ".join(str(elem) for elem in target_stdev_list) + "<br>"
    mail_body += "EndDate: " + end_date + "<br>"
    mail_body += "NumDays: " + num_days + "<br>"

    if len(regress_exec_list) != 0:
        mail_body += "REG_STRING: " + str(reg_string) + "<br>"
        mail_body += "DATAGEN_ARGS: " + str(datagen_args) + "<br>"
        mail_body += "REGDATA_ARGS: " + str(regdata_args) + "<br>"
        mail_body += "REGDATA_PROCESS_FILTER: " + str(regdata_process_filter) + "<br>"

    if max_sum is not None:
        mail_body += "MaxSumIndicators: " + max_sum + "<br>"
    if user_matrix is not None:
        mail_body += "ModelWeights: " + '\n'.join(' '.join(map(str, x)) for x in user_matrix)
        mail_body += "<br>"
    if sort_algo is not None:
        mail_body += "SortAlgo: " + sort_algo
        mail_body += "<br>"
    if max_ttc_threshold is not None:
        mail_body += "MaxTTCThreshold: " + str(max_ttc_threshold) + "<br>"
    if min_vol_threshold is not None:
        mail_body += "MinVolumeThreshold: " + str(min_vol_threshold) + "<br>"
    mail_body += "</p>"

    print("Shortcode: " + shortcode)
    print("ExecLogic: " + execlogic)
    print("Ilist: " + ilist)
    print("ParamFile: " + " ".join(str(elem) for elem in param_file_list))
    print("StartTime: " + start_time)
    print("EndTime: " + end_time)
    print("TargetStdev: " + " ".join(str(elem) for elem in target_stdev_list))
    print("EndDate: " + end_date)
    print("NumDays: " + num_days)
    print("ChooseTopStrats: " + choose_top_strats)

    if max_sum is not None:
        print("MaxSumIndicators: " + max_sum)
    if user_matrix is not None:
        print("ModelWeights: ")
        print(user_matrix)
    if regime_indicator is not None:
        print("RegimeIndicator: " + regime_indicator)
    if num_regimes is not None:
        print("NumRegimes: " + num_regimes)
    work_dir, mail_content, _, _ = get_best_pnl_model(shortcode, execlogic, ilist, param_file_list,
                                                      start_date, end_date,
                                                      start_time, end_time, target_stdev_list,
                                                      choose_top_strats, max_sum, user_matrix, sort_algo,
                                                      regime_indicator=regime_indicator, num_regimes=num_regimes,
                                                      regime_trading_indicator_list=regime_trading_indicator_list,
                                                      regimes_to_trade=regimes_to_trade,
                                                      three_step=three_step,
                                                      dates_for_sim=dates_for_sim,
                                                      training_dates_list=training_dates_list,
                                                      validation_dates_list=validation_dates_list,
                                                      test_dates_list=test_dates_list,
                                                      using_grid=using_grid,
                                                      cv_list=cv_list, cv_combinations=cv_combinations, num_folds=num_folds,
                                                      max_val_three_step=max_val_three_step,
                                                      num_weights_three_step=num_weights_three_step, cname=cname,
                                                      max_ttc_threshold=max_ttc_threshold, min_vol_threshold=min_vol_threshold)

    try:
        os.system("cp " + preprocessing_days_file + " " + work_dir + "/preprocessing_days")
    except:
        pass

    if os.path.exists(work_dir_preprocess):
        shutil.rmtree(work_dir_preprocess)

    mail_body += "<p>WORK_DIR: " + work_dir + "</p><br>"
    mail_body += "\n"

    mail_body += mail_content

    send_email(mail_address, mail_body, work_dir)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', dest='configname', help="the pnl modelling config", type=str, required=True)
    parser.add_argument('--nogrid', dest='using_grid',
                        help='whether to use grid or not', default=True,
                        required=False, action='store_false')

    args = parser.parse_args()
    if args.using_grid and ('GRID_USERNAME' not in os.environ or os.environ['GRID_USERNAME'] == "" or 'GRID_PASSWORD' not in os.environ or os.environ['GRID_PASSWORD'] == ""):
        grid_user = input("Enter username: ")
        if grid_user == "":
            print("Enter valid username")
            sys.exit(1)
        password = getpass.getpass("Enter password: ")
        os.environ['GRID_USERNAME'] = grid_user
        os.environ['GRID_PASSWORD'] = password
    # This command reads the params from the config
    # and also finds the best pnl from the model.
    read_params_from_config(args.configname, args.using_grid)
