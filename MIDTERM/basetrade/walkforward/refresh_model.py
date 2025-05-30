#!/usr/bin/env python

"""
# \file walkforward/refresh_model.py
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#


# INITARGS
# SDate LBackdays
# STime ETime 
# EcoString ?
# RegimeString ?

# TIMESERIESDATA
# TD@Time,Events,Trades

# REGDATA
# RD@ProcessAlgo,PredDurationInSecs

# PREPROCESSING
# PRE@filter,scale

# WEIGHTSMETHOD
# LM FSRR SIGLR

# POSTPROCESSING
# SCALE
"""


import argparse
import subprocess
import sys
import os
import getpass
import time
import signal
import datetime
import time
import json
import shutil

sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.utils.date_utils import calc_next_week_day
from walkforward.utils.date_utils import calc_prev_week_day
from walkforward.utils.date_utils import calc_first_weekday_date_of_month
from walkforward.utils.shortcode_utils import get_tick_ratio_for_dates
from walkforward.wf_db_utils.fetch_config_details import fetch_config_details
from walkforward.wf_db_utils import insert_model_into_db_wf_config_type6
from walkforward.utils.build_model import build_model
from walkforward.utils.get_dates_for_feature_change import get_dates_for_feature_change
from walkforward.wf_db_utils.fetch_latest_model_date import fetch_latest_model_date
from walkforward.wf_db_utils.fetch_strat_from_type6_config_struct_and_date import check_strat_for_config6_and_date
from walkforward.utils.find_best_param_for_model import find_best_param_for_model
from scripts.find_best_pnl_model import get_best_pnl_model
from walkforward.wf_db_utils import dump_strat_for_config_for_day
from walkforward.utils.insert_rescaled_model_for_date import insert_rescaled_model_for_date

def signal_handler(signal, frame):
    print("There is a siginit")
    sys.exit(0)


signal.signal(signal.SIGINT, signal_handler)


# trigger_smart_string definations:
# D1 == every day
# W5 == every_friday
# M1 == first week day of every month
# D1F1 = every day if feature value in a new region .....
# enddate is tradingdate - 1
# start and end time same as trading start and end time

# original_model_file
# lookback is param
# td_string 1000 0 0
# rd_string
# rm_string
# pre_string
# pos_string


def refresh_model(config_name_, trading_date_, numdays_2_refresh_, day_to_model_param_pair_, parent_work_dir, overwrite):
    """
    we are hoping to receive, this one should be triggerred, Tue-Sat
    hence Tue, Wed, Thur, Fri and Mon
    :param config_name_: 
    :param trading_date_: 
    :param numdays_2_refresh_: 
    :param day_to_model_param_pair_: 
    :param parent_work_dir: 
    :param overwrite: 
    :return: 
    """
    should_refresh = False
    this_cfg_ = fetch_config_details(config_name_)
    config_json = json.loads(this_cfg_.config_json)

    config_created_date = config_json['description:'].split()[-1].replace("-","")

    # for now we are using same param file across
    param_list_ = config_json['param_list']

    # this will be the ilist (i.e with all weights set to 1)
    model_list_ = config_json['model_list']

    if 'trigger_string' in config_json:
        trigger = config_json['trigger_string']
    else:
        print("trigger string missing, using D0 as default, generates new model for every monday")
        trigger = "D0"

    # if called on saturday
    if datetime.datetime.strptime(str(trading_date_), '%Y%m%d').weekday() == 5:
        trading_date_ = calc_next_week_day(trading_date_, 1)

    # if called on sunday
    if datetime.datetime.strptime(str(trading_date_), '%Y%m%d').weekday() == 6:
        trading_date_ = calc_next_week_day(trading_date_, 1)

    trigger_string_feature_list = ['STDEV', 'L1SZ', 'VOL']
    trigger_split_list = trigger.split('_')
    if (trigger_split_list[0] in trigger_string_feature_list) and (len(trigger_split_list) == 3):
        """
        # Trigger string is like STDEV_20_0.2
        # Get the list of dates where model need to be refreshed i.e.
        #   only when abs((Y(i) - Y(i-1))/Y(i-1)) > threshold(say 0.2).
        #   Where Y(i) is the stdev for given last days(say 20) from date X(i).
        # X(0) = Latest date when the model was generated.
        # Will be equal to walk start date if running for the first time for a config.

        """
        print(("Trigger String is " + str(trigger)))
        shortcode = config_json['shortcode']
        walk_start_date = config_json['walk_start_date']
        start_time = config_json['start_time']
        end_time = config_json['end_time']
        feature = trigger_split_list[0]
        feature_lookback_days = trigger_split_list[1]
        feature_comparision_threshold = trigger_split_list[2]

        # Get latest date for which model is available in db.
        # It is equal to walk start date if running for first time for a config.
        latest_model_present_in_db = fetch_latest_model_date(config_name_, trading_date_)
        print("Latest Date for which Model is available in DB before",
              str(trading_date_), " =", str(latest_model_present_in_db))

        if latest_model_present_in_db is None:
            print("No Date found in DB for which model is available. Check the trading date passed in arguments. "
                  "For now, using walk start date =", walk_start_date, "as starting date.")
            latest_model_present_in_db = int(walk_start_date)

        # Get list of trigger dates for which model needs to be refreshed
        model_refresh_date_list = get_dates_for_feature_change(
            shortcode, latest_model_present_in_db, trading_date_, start_time, end_time, feature, feature_lookback_days, feature_comparision_threshold)
        print("List of trigger dates = ", model_refresh_date_list)

    # remember we ensuring to use only [0-4] weekdays for evalution
    todo_date_ = trading_date_
    print(numdays_2_refresh_)

    # Creating a list to store tick_changes_encountered; so that model_refresh is called only once for a tick change
    # What do you do if someone starts recompute again from in between, how to check if that tick_change has been encountered earlier;
    # Well for now,ignoring the tick_change on trading_date; date with which this function is called
    tick_changes_encountered = [1]
    tick_ratio_trading_date = get_tick_ratio_for_dates(config_json['shortcode'], trading_date_, config_created_date)
    if tick_ratio_trading_date != 1:
        tick_changes_encountered.append(tick_ratio_trading_date)

    # Creating a list to store model_refresh_dates for which sign_check failed, so that we can insert the model for
    # the previous date on this model_refresh_date; This will help in not trying to refresh again, and assuming it to be
    # the nature of the sign check that the model previous to it would be used, we won't be getting errors, where a model
    # has not been updated for some time. This is better than not having a strat if sign_check fails, right?
    model_refresh_dates_with_failed_sign_check = []
    # Creating a list to track which days have we already refreshed the model for
    already_refreshed_date_set = set()
    for i in range(0, int(numdays_2_refresh_)):
        print(("check for ith day " + str(i)))
        print("current date = ", todo_date_)
        should_refresh = False

        # start of trigger algos
        # monday is zero, sunday is 6
        if trigger == "D0":
            print("trigger logic is set to D0")
            # If the current date is not Monday, then compute model for Monday.
            # So, in any caes compute model for Monday only.
            should_refresh = True
            model_refresh_date = calc_prev_week_day(
                todo_date_, datetime.datetime.strptime(str(todo_date_), '%Y%m%d').weekday())

        if trigger == "M1":
            print("trigger logic is set to M1")
            # If todo_date_ is not first weekday of the month, then compute model for first weekday of the month.
            # So, in any case compute moodel for first weekday of the month.
            should_refresh = True
            model_refresh_date = calc_first_weekday_date_of_month(todo_date_)

        if (trigger_split_list[0] in trigger_string_feature_list) and (len(trigger_split_list) == 3):
            print(("trigger logic is set to " + trigger))
            # If the current date is not a date where model should be refreshed,
            # then refresh model for the last date on which the model was supposed to be refreshed.
            should_refresh = True
            model_refresh_date = max(filter(lambda x: x <= todo_date_, model_refresh_date_list))

        tick_ratio = get_tick_ratio_for_dates(config_json['shortcode'], todo_date_,config_created_date)

        if tick_ratio not in tick_changes_encountered:
            tick_changes_encountered.append(tick_ratio)
            if int(todo_date_) < int(config_created_date):

                print("tick_change happened on " + str(todo_date_) +" so Model Refresh triggerred for " +\
                   str(calc_next_week_day(todo_date_)))
                model_refresh_date = calc_next_week_day(todo_date_)



            else:
                date_tocheck =  calc_prev_week_day(todo_date_)
                flag = True
                "Keeping a count just as a precaution"
                count = 0
                while flag and count < 5000:
                    tick_ratio_for_check = get_tick_ratio_for_dates(config_json['shortcode'],date_tocheck,config_created_date)
                    if tick_ratio_for_check != tick_ratio:
                        model_refresh_date = calc_next_week_day(date_tocheck)
                        flag = False
                    date_tocheck = calc_prev_week_day(date_tocheck)
                    count += 1


        if model_refresh_date < int(config_json['walk_start_date']):
            model_refresh_date = int(config_json['walk_start_date'])

        # end of trigger algos
        # Check if the strat already exists for the model_refresh_date or not. If yes, then we may not need to refresh.

        # By design, the model_refresh_date passed here is always the date where model is expected to be present;
        # only exception being on tick_change_date and walk_start_date as of now; we want to overwrite walk_start_date
        # because the model_on_walk_start_date is useless otherwise; the demerit being in every run it will always overwrite
        # for walk start date; assuming ideally one won't be calling run_compute_strat again for  walk_start_date
        to_bypass = ((model_refresh_date == calc_next_week_day(todo_date_)) or (model_refresh_date == int(config_json['walk_start_date'])))
        (exists, e_date) = check_strat_for_config6_and_date(this_cfg_.configid, model_refresh_date,to_bypass)

        if exists:
            print("Model exists for " + str(model_refresh_date))
            if not overwrite:
                should_refresh = False


        "If it has already been attempted once , we should not attempt again in any case"
        if model_refresh_date in already_refreshed_date_set:
            print("model was already refreshed for " + str(model_refresh_date) + " in this run earlier.")
            should_refresh = False




        if should_refresh:
            print(("Current date is " + str(todo_date_) + " and Model Refresh triggerred for " + str(model_refresh_date)))
            already_refreshed_date_set.add(model_refresh_date)
            if (config_json["reg_string"].split())[0] == "PNL":
                work_dir, _, best_model, best_param_indx = \
                    get_best_pnl_model(config_json['shortcode'], config_json['execlogic'],
                                       model_list_[0], param_list_, model_refresh_date,
                                       config_json['ddays_string'], config_json['start_time'], config_json['end_time'],
                                       config_json['model_process_string'].split()[1:],
                                       config_json['choose_top_strats'], config_json['max_sum'],
                                       config_json['user_matrix'], config_json['sort_algo'],
                                       regime_indicator=config_json['regime_indicator'],
                                       num_regimes=config_json['num_regimes'],
                                       three_step=config_json['three_step_optim'],
                                       skip_days_file=config_json['skip_days'])

                if best_param_indx != -1:
                    insert_model_into_db_wf_config_type6.insert_model_coeffs_in_db(model_list_[0], best_model,
                                                                                   this_cfg_.configid, int(model_refresh_date))

                    if model_list_[0] != "INVALID" and param_list_[best_param_indx] != "INVALID":
                        print(("For DATE: " + str(model_refresh_date) + " And CONFIG: " + config_name_ +
                               "\nMODEL : " + model_list_[0] + "\nPARAM: " + param_list_[best_param_indx]))
                        dump_strat_for_config_for_day.dump_strat_for_config_for_day(model_list_[0], param_list_[best_param_indx],
                                                                                    int(model_refresh_date), config_name_, overwrite)

                if os.path.exists(work_dir):
                    shutil.rmtree(work_dir, ignore_errors=True)


            else:

                sign_check_string = "0"
                if 'sign_check' in config_json:
                    sign_check_string = config_json['sign_check']

                if 'is_structured' in config_json and int(config_json['is_structured']) == 1:
                    # this is model corresponding to the structured strat
                    print ('STRUCTURED', len(config_json['model_list']))

                    if 'model_list' in config_json and len(config_json['model_list']) > 0:
                        (valid_model_check, work_dir, model_file) = ('INVALID', parent_work_dir, config_json['model_list'][0])
                    else:
                        sys.stderr.write('For structured global strategy model file entry not found')

                else:
                    (valid_model_check, work_dir, model_file) = build_model(config_json['shortcode'], this_cfg_.configid,
                                                                        model_refresh_date, config_json['ddays_string'],
                                                                        config_json['start_time'], config_json['end_time'],
                                                                        config_json['td_string'], config_json['rd_string'],
                                                                        config_json['rdata_process_string'],
                                                                        config_json['reg_string'],
                                                                        config_json['model_process_string'],
                                                                        sign_check_string, config_json, parent_work_dir)

                # Insert coeffecients only if sign check of model has passed
                if valid_model_check:
                    insert_model_into_db_wf_config_type6.insert_model_coeffs_in_db(model_list_[0], model_file, this_cfg_.configid, int(model_refresh_date))

                    if 'param_lookback_days' in config_json:
                        lookback_days = int(config_json['param_lookback_days'])
                    else:
                        print("Lookback Days not provided, using 20 as default")
                        lookback_days = 20

                    end_date = calc_prev_week_day(model_refresh_date, 1)
                    start_date = calc_prev_week_day(end_date, lookback_days)
                    model, param = find_best_param_for_model(
                        [model_file], config_json['param_list'], config_json, start_date, end_date)

                    if model_list_[0] != "INVALID" and param != "INVALID":
                        print(("For DATE: " + str(model_refresh_date) + " And CONFIG: " + config_name_ +
                               "\nMODEL : " + model_list_[0] + "\nPARAM: " + param))
                        dump_strat_for_config_for_day.dump_strat_for_config_for_day(model_list_[0], param, model_refresh_date, config_name_,
                                                                                    overwrite)
                else:
                    # Right now valid_model_check is synonymous to sign_check; and we are creating a list of
                    # model_refresh_dates_with_failed_sign_check, so that we can insert the previous existing model for
                    # this model_refresh_date; if more checks are introduced we will have to change the logic accordingly
                    # depending on we would want to insert the previous existing model for that check

                    print("Sign Check failed for model so not inserting the model to DB\n")
                    model_refresh_dates_with_failed_sign_check.append(model_refresh_date)

                    # if we don't want to update in case sign_check_fails and want to update only for tick_change_dates
                    """
                    if model_refresh_date == calc_next_week_day(todo_date_):
                        model_refresh_dates_with_failed_sign_check.append(model_refresh_date)
                    """
                if work_dir != None and parent_work_dir != work_dir:
                    if os.path.exists(work_dir):
                        shutil.rmtree(work_dir, ignore_errors=True)

        
        else:
            print(("skipped for " + str(todo_date_)))

        todo_date_ = calc_prev_week_day(todo_date_, 1)

    for date_to_insert_prev_model in model_refresh_dates_with_failed_sign_check:
        insert_rescaled_model_for_date(this_cfg_, date_to_insert_prev_model)
