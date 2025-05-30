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
import time

sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.utils.date_utils import calc_next_week_day
from walkforward.utils.date_utils import calc_prev_week_day
from walkforward.utils.date_utils import calc_first_weekday_date_of_month
from walkforward.wf_db_utils.fetch_config_details import fetch_config_details
from walkforward.wf_db_utils import insert_model_into_db_wf_config_type6
from walkforward.wf_db_utils import insert_model_into_db_wf_config_type7
#from walkforward.utils.build_model import build_model
from walkforward.utils.get_dates_for_feature_change import get_dates_for_feature_change
from walkforward.wf_db_utils.fetch_latest_model_date import fetch_latest_model_date
from walkforward.wf_db_utils.fetch_strat_from_type6_config_struct_and_date import check_strat_for_config6_and_date
from walkforward.utils.find_best_param_for_model import find_best_param_for_model
from scripts.find_best_pnl_model import get_best_pnl_model
from walkforward.wf_db_utils import dump_strat_for_config_for_day
from scripts.get_di_universe import WriteShortCodeListtoFile
from walkforward.wf_db_utils.fetch_dump_model import fetch_model_desc_from_config_id_and_date
from walkforward.definitions import execs
from scripts.get_di_ilist import MakeIlist
from scripts.generate_di_models import build_model
from scripts.generate_di_params import GetHighestTradedDI
from scripts.generate_di_params import GetParamsForShortCode
from scripts.generate_di_params import GetTickSize
from scripts.get_di_universe import GetShortCodeList


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


def refresh_model_for_di(config_name_, trading_date_, numdays_2_refresh_, day_to_model_param_pair_, parent_work_dir,
                         overwrite):
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
            shortcode, latest_model_present_in_db, trading_date_, start_time, end_time, feature, feature_lookback_days,
            feature_comparision_threshold)
        print("List of trigger dates = ", model_refresh_date_list)

    # remember we ensuring to use only [0-4] weekdays for evalution
    todo_date_ = trading_date_

    # modelString=

    print(numdays_2_refresh_)

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

        if model_refresh_date < int(config_json['walk_start_date']):
            model_refresh_date = int(config_json['walk_start_date'])

        # end of trigger algos
        # Check if the strat already exists for the model_refresh_date or not. If yes, then we may not need to refresh.

        # By design, the model_refresh_date passed here is always the date where model is expected to be present.
        (exists, e_date) = check_strat_for_config6_and_date(this_cfg_.configid, model_refresh_date)

        if exists:  # If does not exists then refresh anyway.
            if overwrite:
                # If overwrite is true, check if we already refreshed in this run. If yes, then do not refresh.
                if model_refresh_date in already_refreshed_date_set:
                    print("Either model should not refresh for this date or model was already "
                          "refreshed for this date in this run earlier.")
                    should_refresh = False
            else:  # If not overwrite, then do not refresh.
                should_refresh = False
        if should_refresh:
            model_file_init_ = WriteShortCodeListtoFile(str(model_refresh_date), this_cfg_.configid)
            model_list_ = []
            model_list_.append(model_file_init_)
            param_list_ = []
            CommonParamFile = "/media/shared/ephemeral16/diwakar/GE/im_strats/common_param_0.3"
            param_list_.append(CommonParamFile)

            insert_model_into_db_wf_config_type6.insert_model_coeffs_in_db_for_dicbt(model_list_[0], model_list_[0],
                                                                                     this_cfg_.configid,
                                                                                     int(model_refresh_date), True)
            if model_list_[0] != "INVALID" and param_list_[0] != "INVALID":
                print(("For DATE: " + str(model_refresh_date) + " And CONFIG: " + config_name_ +
                       "\nMODEL : " + model_list_[0] + "\nPARAM: " + param_list_[0]))
                dump_strat_for_config_for_day.dump_strat_for_config_for_day(model_list_[0],
                                                                            param_list_[0],
                                                                            int(model_refresh_date),
                                                                            config_name_, overwrite)

                model_string = fetch_model_desc_from_config_id_and_date(this_cfg_.configid, int(model_refresh_date))
                shortCodeList = []
                print("Model String : ")
                print(model_string)
                for line in model_string.split("\n"):
                    indArray = line.split()
                    if len(indArray) < 4:
                        continue
                    print("IndArray")
                    print(indArray)
                    shc = indArray[2]
                    shcVolume = indArray[3]
                    shortCodeList.append([shc, shcVolume])
                index = 0
                extraShcList = []
                redundantShcList = []
                if 0:  # update_mode == "RegenerateUniverse"
                    shortCodeListFromWorkDir = os.popen(
                        "ls " + work_dir + dateNow + "/ | grep DI").read().split("\n")
                    shortCodeListFromWorkDir.remove('')
                    print("ShListWorkDir", shortCodeListFromWorkDir)
                    print("ShList", shortCodeList)
                    shlist2 = []
                    for liner in shortCodeList:
                        shlist2.append(liner[0])
                    for item in shortCodeList:
                        if item[0] not in shortCodeListFromWorkDir:
                            extraShcList.append(item)
                    for item in shortCodeListFromWorkDir:
                        if item not in shlist2:
                            redundantShcList.append(item)
                            os.system("rm -r " + work_dir + dateNow + "/" + str(item))
                else:
                    extraShcList = shortCodeList

                regenerateModels = 1
                regenerateParams = 1
                if 0:  # update_mode == "RegenerateParams"
                    regenerateModels = 0
                if 0:  # update_mode == "RegenerateModels"
                    regenerateParams = 0
                if 1:  # update_mode == "RegeneateAll"
                    regenerateModels = 1
                    regenerateParams = 1

                StratList = []
                universe = []
                leaderShc = GetHighestTradedDI(shortCodeList[0][0], str(model_refresh_date), universe)

                for shc in shortCodeList:
                    ilist = MakeIlist(int(model_refresh_date), shc[0], shortCodeList)
                    if not os.path.isdir(parent_work_dir):
                        os.system("mkdir " + parent_work_dir)
                    if not os.path.isdir(parent_work_dir + str(model_refresh_date) + "/"):
                        os.system("mkdir " + parent_work_dir + str(model_refresh_date) + "/")
                    if not (os.path.isdir(parent_work_dir + str(model_refresh_date) + "/" + shc[0])):
                        os.system("mkdir " + parent_work_dir + str(model_refresh_date) + "/" + shc[0])

                    is_ilist_changed = True
                    original_common_ilist = ""
                    print(parent_work_dir + shc[0] + "/ilist_file")
                    if os.path.isfile(parent_work_dir + shc[0] + "/ilist_file"):
                        fo = open(parent_work_dir + shc[0] + "/ilist_file", "r")
                        original_common_ilist = fo.read()
                        print("Common Ilist")
                        print(original_common_ilist)
                        fo.close()
                    if ilist.strip() == original_common_ilist.strip():
                        print("Same as original Ilist")
                        is_ilist_changed = False
                    else:
                        print("Ilist has been changed")
                    print("Original Common Ilist")
                    print()
                    print(original_common_ilist)
                    print("New Ilist")
                    print(ilist)
                    print()
                    if not os.path.isdir(parent_work_dir):
                        os.system("mkdir " + parent_work_dir)
                    if not os.path.isdir(parent_work_dir + "/" + shc[0]):
                        os.system("mkdir " + parent_work_dir + "/" + shc[0])
                    ilist_file = parent_work_dir + str(model_refresh_date) + "/" + \
                        shc[0] + "/ilist_file_date_" + str(model_refresh_date)
                    common_ilist_file = parent_work_dir + shc[0] + "/ilist_file"
                    fo = open(ilist_file, "w")
                    fo.write(ilist)
                    fo.close()
                    fo = open(common_ilist_file, "w")
                    fo.write(ilist)
                    fo.close()
                    final_model_file = parent_work_dir + \
                        str(model_refresh_date) + "/" + shc[0] + "/finalmodel_file_date_" + str(model_refresh_date)

                    childConfigName = this_cfg_.cname + "_child_" + str(index)
                    child_cfg_ = fetch_config_details(childConfigName)
                    child_config_json = json.loads(this_cfg_.config_json)
                    (exists, e_date) = check_strat_for_config6_and_date(child_cfg_.configid, model_refresh_date)
                    if exists:  # If does not exists then refresh anyway.
                        if overwrite:
                            # If overwrite is true, check if we already refreshed in this run. If yes, then do not refresh.
                            if model_refresh_date in already_refreshed_date_set:
                                print("Either model should not refresh for this date or model was already "
                                      "refreshed for this date in this run earlier.")
                                should_refresh = False
                        else:  # If not overwrite, then do not refresh.
                            should_refresh = False
                    if should_refresh:
                        print(
                            ("Current date is " + str(todo_date_) + " and Model Refresh triggerred for " + str(
                                model_refresh_date)))
                        already_refreshed_date_set.add(model_refresh_date)
                        if (child_config_json["reg_string"].split())[0] == "PNL":
                            work_dir, _, best_model, best_param_indx = \
                                get_best_pnl_model(shc, child_config_json['execlogic'],
                                                   ilist_file, param_list_, model_refresh_date,
                                                   child_config_json['ddays_string'], child_config_json['start_time'],
                                                   child_config_json['end_time'],
                                                   child_config_json['model_process_string'].split()[1:],
                                                   child_config_json['choose_top_strats'], child_config_json['max_sum'],
                                                   child_config_json['user_matrix'], child_config_json['sort_algo'],
                                                   regime_indicator=child_config_json['regime_indicator'],
                                                   num_regimes=child_config_json['num_regimes'],
                                                   three_step=child_config_json['three_step_optim'],
                                                   skip_days_file=child_config_json['skip_days'])

                            if best_param_indx != -1:
                                insert_model_into_db_wf_config_type6.insert_model_coeffs_in_db_(best_model,
                                                                                                best_model,
                                                                                                child_cfg_.configid,
                                                                                                int(model_refresh_date), True)

                                if best_model != "INVALID" and param_list_[best_param_indx] != "INVALID":
                                    print(("For DATE: " + str(model_refresh_date) + " And CONFIG: " + child_cfg_.cname +
                                           "\nMODEL : " + best_model + "\nPARAM: " + param_list_[best_param_indx]))
                                    dump_strat_for_config_for_day.dump_strat_for_config_for_day(best_model,
                                                                                                param_list_[
                                                                                                    best_param_indx],
                                                                                                int(model_refresh_date),
                                                                                                child_cfg_.cname, overwrite)

                            if os.path.exists(work_dir):
                                shutil.rmtree(work_dir, ignore_errors=True)

                        else:

                            sign_check_string = "0"
                            if 'sign_check' in child_config_json:
                                sign_check_string = child_config_json['sign_check']
                                shortCodeList = shortCodeList

                            num_lookback_days = child_config_json['ddays_string']
                            start_time = child_config_json['start_time']
                            end_time = child_config_json['end_time']
                            td_string = child_config_json['td_string']
                            ptime, events, trades = td_string.split()
                            rd_string = child_config_json['rd_string']
                            pred_algo, pred_duration = rd_string.split()
                            data_filter = child_config_json['rdata_process_string']
                            reg_string = child_config_json['reg_string']
                            target_stdev = child_config_json['model_process_string'].split()[1]

                            # ignore the below block needs to be changed
                            init_model_file = parent_work_dir + shc[0] + "/ilist_file"
                            param_file = parent_work_dir + str(model_refresh_date) + "/" + shc[0] + "/param_file"
                            #current_date = now.strftime("%Y%m%d")

                            final_model_file = build_model(shc[0], ilist_file, str(model_refresh_date), num_lookback_days, start_time, end_time, ptime,
                                                           events, trades, pred_duration,
                                                           pred_algo, data_filter, reg_string, parent_work_dir, is_ilist_changed, child_cfg_.configid, child_config_json)

                            temp_model_file = parent_work_dir + \
                                str(model_refresh_date) + "/" + shc[0] + "/temp_model_file"
                            os.system("scp " + final_model_file + " " + temp_model_file)
                            prev_date = os.popen(
                                "/home/dvctrader/basetrade_install/bin/calc_prev_day " + str(model_refresh_date) + " 60").read()
                            model_stdev = os.popen(
                                "/home/dvctrader/basetrade/ModelScripts/get_stdev_model.pl " + final_model_file + " " + prev_date + " " + str(model_refresh_date) + " " + "BRT_905 BRT_1540 | awk '{print $1}'").read()
                            print("Model Stdev : ")
                            print(model_stdev)
                            print("Target Stdev In Ticks:")
                            print(target_stdev)
                            model_scale = float(target_stdev)*GetTickSize(shc[0],str(model_refresh_date)) / float(model_stdev)

                            print("Model Before Scaling :")
                            print()
                            with open(final_model_file, 'r') as f:
                                print(f.read())
                            print("Scaling MOdel By :", model_scale)
                            os.system(
                                "~/basetrade/ModelScripts/rescale_model.pl " + temp_model_file + " " + final_model_file + " " + str(
                                    model_scale))
                            print("Model After Scaling")
                            print()
                            with open(final_model_file, 'r') as f:
                                print(f.read())

                            print("")
                            final_param_file = parent_work_dir + \
                                str(model_refresh_date) + "/" + shc[0] + "/param_file_date_" + str(model_refresh_date)

                            if regenerateParams == 1 and shc in extraShcList:
                                baseDate = "20170801"
                                baseUts = 100
                                shortCode = shc[0]
                                universe = []
                                baseparamFile = "/media/shared/ephemeral16/devendra/BMF/param_DI1_common"
                                f = open(baseparamFile, "r")
                                baseParam = f.read()
                                f.close()
                                final_param_file = GetParamsForShortCode(shortCode, str(model_refresh_date), baseParam, leaderShc,
                                                                         baseDate, baseUts,
                                                                         parent_work_dir + str(model_refresh_date) + "/", child_cfg_.configid)
                            with open(final_param_file, 'r') as f2:
                                print(f2.read())
                            print("")
                            valid_model_check = 1

                            '''(valid_model_check, work_dir, model_file) = build_model(shc,
                                                                                        child_cfg_.configid,
                                                                                        model_refresh_date,
                                                                                        child_config_json['ddays_string'],
                                                                                        child_config_json['start_time'],
                                                                                        child_config_json['end_time'],
                                                                                        child_config_json['td_string'],
                                                                                        child_config_json['rd_string'],
                                                                                        child_config_json[
                                                                                            'rdata_process_string'],
                                                                                        child_config_json['reg_string'],
                                                                                        child_config_json[
                                                                                            'model_process_string'],
                                                                                        sign_check_string, child_config_json,
                                                                                        parent_work_dir)'''

                            # Insert coeffecients only if sign check of model has passed

                            if valid_model_check:
                                insert_model_into_db_wf_config_type6.insert_model_coeffs_in_db(final_model_file,
                                                                                               final_model_file,
                                                                                               child_cfg_.configid,
                                                                                               int(model_refresh_date), True)

                                if 'param_lookback_days' in child_config_json:
                                    lookback_days = int(child_config_json['param_lookback_days'])
                                else:
                                    print("Lookback Days not provided, using 20 as default")
                                    lookback_days = 20

                                end_date = calc_prev_week_day(str(model_refresh_date), 1)
                                start_date = calc_prev_week_day(end_date, lookback_days)
                                # model, param = find_best_param_for_model(
                                #   [model_file], child_config_json['param_list'], child_config_json, start_date, end_date)

                                if final_model_file != "INVALID" and final_param_file != "INVALID":
                                    print(("For DATE: " + str(model_refresh_date) + " And CONFIG: " + child_cfg_.cname +
                                           "\nMODEL : " + final_model_file + "\nPARAM: " + final_param_file))
                                    dump_strat_for_config_for_day.dump_strat_for_config_for_day(final_model_file, final_param_file,
                                                                                                model_refresh_date,
                                                                                                child_cfg_.cname,
                                                                                                overwrite)

                            # if work_dir != None and parent_work_dir != work_dir:
                            #    if os.path.exists(work_dir):
                            #        shutil.rmtree(work_dir, ignore_errors=True)
                    index += 1

        else:
            print(("skipped for " + str(todo_date_)))

        todo_date_ = calc_prev_week_day(todo_date_, 1)
