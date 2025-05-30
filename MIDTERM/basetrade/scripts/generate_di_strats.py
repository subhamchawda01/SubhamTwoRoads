#!/usr/bin/python

from datetime import date
import sys
import os
import subprocess
import math
import operator
import heapq
import argparse
import getpass
import time
import datetime


sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.definitions import execs
from scripts.get_di_universe import GetDV01Volume
from scripts.get_di_universe import GetShortCodeList
from scripts.get_di_sources import GetSources
from scripts.get_di_ilist import MakeIlist
from scripts.generate_di_models import build_model
from scripts.generate_di_params import GetHighestTradedDI
from scripts.generate_di_params import GetParamsForShortCode
from walkforward.utils.date_utils import calc_first_weekday_date_of_month
from walkforward.utils.date_utils import calc_prev_week_day
from walkforward.utils.date_utils import calc_next_week_day
from walkforward.wf_db_utils.db_handles import set_backtest
from walkforward.create_config_from_ifile import instruction_set
from walkforward.create_config_from_ifile import create_config
from walkforward.create_config_from_ifile import check_seq2conf_sensitivity
from walkforward.create_config_from_ifile import run_simulations
from walkforward.compute_strat_for_config_and_date import compute_strat_for_config_and_date

import numpy


from walkforward.utils.prepare_rdata import convert_t2d
from walkforward.utils.process_rdata import apply_filters
from walkforward.utils.prepare_tdata import run_datagen
from walkforward.utils.learn_signal_weights import run_method, run_fill_based_model_generator
from walkforward.wf_db_utils.utils import FileExistsWithSize


def StructStrategyGenerate(StratList, CombinedShortCode, TradingManagerName, ExecLogic, CommonParamFile, StartTime, EndTime, ProgId, workDir):
    StratString = ""
    StratString += "STRUCTURED_TRADING " + CombinedShortCode + " " + TradingManagerName + \
        "-StructuredGeneralTrading-" + ExecLogic + " " + CommonParamFile + " " + StartTime + " " + EndTime + " " + ProgId
    for line in StratList:
        StratString += "\n" + "STRATEGYLINE " + str(line[0][0]) + " " + str(line[1]) + " " + str(line[2])
    print(StratString)
    if not os.path.isdir(workDir):
        os.system("mkdir " + workDir)
    innerStrategyFile = workDir + "innerStrategyFile"
    fo = open(innerStrategyFile, "w")
    fo.write(StratString)
    fo.close()

    return innerStrategyFile


def GenerateDatedStrategyForShortCode(dateNow, current_date, reg_string, pred_duration, pred_algo, data_filter, start_time, end_time, td_string, walk_start_date, num_lookback_days, work_dir, update_frequency, target_stdev, shc, regenerateModels, regenerateParams, StratList, leaderShc, shortCodeList, extraShcList, redundantShcList):
    ilist = MakeIlist(dateNow, shc[0], shortCodeList)
    if not os.path.isdir(work_dir):
        os.system("mkdir " + work_dir)
    if not os.path.isdir(work_dir + dateNow + "/"):
        os.system("mkdir " + work_dir + dateNow + "/")
    if not (os.path.isdir(work_dir + dateNow + "/" + shc[0])):
        os.system("mkdir " + work_dir + dateNow + "/" + shc[0])
    is_ilist_changed = True
    original_common_ilist = ""
    print(work_dir + shc[0] + "/ilist_file")
    if os.path.isfile(work_dir + shc[0] + "/ilist_file"):
        fo = open(work_dir + shc[0] + "/ilist_file", "r")
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
    if not os.path.isdir(work_dir):
        os.system("mkdir " + work_dir)
    if not os.path.isdir(work_dir + "/" + shc[0]):
        os.system("mkdir " + work_dir + "/" + shc[0])
    ilist_file = work_dir + dateNow + "/" + shc[0] + "/ilist_file"
    common_ilist_file = work_dir + shc[0] + "/ilist_file"
    fo = open(ilist_file, "w")
    fo.write(ilist)
    fo.close()
    fo = open(common_ilist_file, "w")
    fo.write(ilist)
    fo.close()
    final_model_file = work_dir + dateNow + "/" + shc[0] + "/finalmodel_file"
    ptime, events, trades = td_string.split()
    #pred_algo, pred_duration = rd_string.split()
    if regenerateModels == 1 and shc in extraShcList:
        final_model_file = build_model(shc[0], ilist_file, dateNow, num_lookback_days, start_time, end_time, ptime,
                                       events, trades, pred_duration,
                                       pred_algo, data_filter, reg_string, work_dir, is_ilist_changed)
    temp_model_file = work_dir + dateNow + "/" + shc[0] + "/temp_model_file"
    os.system("scp " + final_model_file + " " + temp_model_file)
    prev_date = os.popen("/home/dvctrader/basetrade_install/bin/calc_prev_day " + dateNow + " 10").read()
    model_stdev = os.popen(
        "/home/dvctrader/basetrade/ModelScripts/get_stdev_model.pl " + final_model_file + " " + prev_date + " " + dateNow + " " + "BRT_905 BRT_1540 | awk '{print $1}'").read()
    model_scale = float(target_stdev) / float(model_stdev)
    print("Model Before Scaling :")
    print()
    with open(final_model_file, 'r') as f:
        print(f.read())
    print("Scaling MOdel By :", model_scale)
    os.system("~/basetrade/ModelScripts/rescale_model.pl " + temp_model_file + " " + final_model_file + " " + str(
        model_scale))
    print("Model After Scaling")
    print()
    with open(final_model_file, 'r') as f:
        print(f.read())

    print("")
    final_param_file = work_dir + dateNow + "/" + shc[0] + "/param_file"
    if regenerateParams == 1 and shc in extraShcList:
        baseDate = "20170801"
        baseUts = 100
        shortCode = shc[0]
        dateNow = dateNow
        universe = []
        baseparamFile = "/media/shared/ephemeral16/devendra/BMF/param_DI1_common"
        f = open(baseparamFile, "r")
        baseParam = f.read()
        f.close()
        final_param_file = GetParamsForShortCode(shortCode, dateNow, baseParam, leaderShc, baseDate, baseUts,
                                                 work_dir + dateNow + "/")
    with open(final_param_file, 'r') as f2:
        print(f2.read())
    print("")
    return [shc, final_model_file, final_param_file]


def CreateIfileArtificially(shortcode, exec_logic, start_time, end_time, strat_type, config_type, type, walk_start_date, ddays_string, trigger_string, td_string, rd_string, rdata_process_string, reg_string, model_process_string, init_model_file, param_file, work_dir, cname, sname, max_child_shortcodes, is_structured, user_matrix):
    IfileString = ""
    if shortcode is not None:
        IfileString += "SHORTCODE\n" + shortcode + "\n\n"
    if exec_logic is not None:
        IfileString += "EXEC_LOGIC\n" + exec_logic + "\n\n"
    if start_time is not None:
        IfileString += "START_TIME\n" + start_time + "\n\n"
    if end_time is not None:
        IfileString += "END_TIME\n" + end_time + "\n\n"
    if strat_type is not None:
        IfileString += "STRAT_TYPE\n" + strat_type + "\n\n"
    if config_type is not None:
        IfileString += "CONFIG_TYPE\n" + str(config_type) + "\n\n"
    if type is not None:
        IfileString += "TYPE\n" + type + "\n\n"
    if init_model_file is not None:
        IfileString += "INIT_MODEL_FILE\n" + init_model_file + "\n\n"
    if param_file is not None:
        IfileString += "PARAM_FILE\n" + param_file + "\n\n"
    if walk_start_date is not None:
        IfileString += "WALK_START_DATE\n" + walk_start_date + "\n\n"
    if ddays_string is not None:
        IfileString += "DDAYS_STRING\n" + ddays_string + "\n\n"
    if trigger_string is not None:
        IfileString += "TRIGGER_STRING\n" + trigger_string + "\n\n"
    if td_string is not None:
        IfileString += "TD_STRING\n" + td_string + "\n\n"
    if rd_string is not None:
        IfileString += "RD_STRING\n" + rd_string + "\n\n"
    if rdata_process_string is not None:
        IfileString += "RDATA_PROCESS_STRING\n" + rdata_process_string + "\n\n"
    if reg_string is not None:
        IfileString += "REG_STRING\n" + reg_string + "\n\n"
    if model_process_string is not None:
        IfileString += "MODEL_PROCESS_STRING\n" + model_process_string + "\n\n"
    if cname is not None:
        IfileString += "CNAME\n" + cname + "\n\n"
    if sname is not None:
        IfileString += "SNAME\n" + sname + "\n\n"
    if max_child_shortcodes is not None:
        IfileString += "MAX_CHILD_SHORTCODES\n" + max_child_shortcodes + "\n\n"
    if is_structured is not None:
        IfileString += "IS_STRUCTURED\n" + is_structured + "\n\n"
    if user_matrix is not None:
        IfileString += "MODEL_WEIGHTS\n"
        for intArray in user_matrix:
            line = " ".join(map(str, intArray))
            IfileString += line + "\n"
        IfileString += "\n"

    if not os.path.isdir(work_dir):
        os.system("mkdir " + work_dir)
    if not os.path.isdir(work_dir + "/" + shortcode):
        os.system("mkdir " + work_dir + "/" + shortcode)

    iFileName = work_dir + shortcode + "/iFile"

    fo = open(iFileName, "w")
    # print()
    fo.write(IfileString)
    fo.close()
    return iFileName


def AddConfigtoDB(iFileName, lstrat, writeAgain, keep_cname, backtest):
    print("IfileName: " + iFileName)
    iset = instruction_set(iFileName, lstrat)
    overwrite = writeAgain != 0
    lstrat = lstrat
    keep_cname = keep_cname

    [iset2, cfg] = create_config(iset, overwrite, lstrat, keep_cname, backtest)

    return [iset2, cfg]
    #create_config_output=os.popen("~/basetrade/walkforward/create_config_from_ifile.py -ifile "+iFileName+" --backtest 1 ").read()
    # return create_config_output


def GenerateStratForDate(dateNow, current_date, reg_string, pred_duration, pred_algo, data_filter, start_time,
                         end_time, td_string, walk_start_date, num_lookback_days, work_dir, trigger_string,
                         target_stdev, update_mode):
    if trigger_string == "M1":
        dateNow = str(calc_first_weekday_date_of_month(dateNow))
    if trigger_string == "D0":
        dateNow = str(calc_prev_week_day(
            dateNow, datetime.datetime.strptime(str(dateNow), '%Y%m%d').weekday()))
    print("Mid Iterations ", dateNow, current_date)
    shortCodeList = GetShortCodeList(dateNow)
    extraShcList = []
    redundantShcList = []

    if update_mode == "RegenerateUniverse":
        shortCodeListFromWorkDir = os.popen("ls " + work_dir + dateNow + "/ | grep DI").read().split("\n")
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
    if update_mode == "RegenerateParams":
        regenerateModels = 0
    if update_mode == "RegenerateModels":
        regenerateParams = 0
    if update_mode == "RegeneateAll":
        regenerateModels = 1
        regenerateParams = 1

    StratList = []
    universe = []
    leaderShc = GetHighestTradedDI(shortCodeList[0][0], dateNow, universe)
    for shc in shortCodeList:
        init_model_file = work_dir + shc[0] + "/ilist_file"
        param_file = work_dir + dateNow + "/" + shc[0] + "/param_file"
        Strat = GenerateDatedStrategyForShortCode(dateNow, current_date, reg_string, pred_duration, pred_algo,
                                                  data_filter, start_time, end_time, td_string, walk_start_date,
                                                  num_lookback_days, work_dir, trigger_string, target_stdev, shc,
                                                  regenerateModels, regenerateParams, StratList, leaderShc, shortCodeList, extraShcList, redundantShcList)
        StratList.append(Strat)
        # IFile
        shortcode = shc[0]
        exec_logic = "PriceBasedAggressiveTrading"
        # name = None
        start_time = "BRT_905"
        end_time = "BRT_1540"
        strat_type = "Regular"
        config_type = "7"
        type = "S"
        walk_start_date = walk_start_date
        ddays_string = num_lookback_days
        trigger_string = trigger_string
        td_string = td_string
        rd_string = pred_algo + " " + pred_duration
        rdata_process_string = data_filter
        reg_string = reg_string
        model_process_string = "0 " + target_stdev

    CombinedShortCode = "DI1X"
    TradingManagerName = "DI1TradingManager"
    ExecLogic = "PriceBasedAggressiveTrading"
    CommonParamFile = "/media/shared/ephemeral16/diwakar/GE/im_strats/common_param_0.3"
    StartTime = "BRT_905"
    EndTime = "BRT_1540"
    ProgId = "1001"
    if not os.path.isdir(work_dir):
        os.system("mkdir " + work_dir)
    if not os.path.isdir(work_dir + dateNow + "/"):
        os.system("mkdir " + work_dir + dateNow + "/")

    innerStrategyName = StructStrategyGenerate(StratList, CombinedShortCode, TradingManagerName, ExecLogic,
                                               CommonParamFile, StartTime, EndTime, ProgId, work_dir + dateNow + "/")

    outerStrategyFile = work_dir + dateNow + "/outerStrategyFile"
    outerString = "STRUCTURED_STRATEGYLINE " + innerStrategyName + " " + ProgId + "\n"
    fo = open(outerStrategyFile, "w")
    print(outerString)
    fo.write(outerString)
    fo.close()
    #dateNow = os.popen("/home/dvctrader/basetrade_install/bin/calc_next_day " + dateNow + " " + update_frequency).read()
    dateNow = os.popen("/home/dvctrader/basetrade_install/bin/calc_prev_week_day " + dateNow + " 1").read()

    return dateNow


def read_params_from_config(args):
    '''
#    Reads the pnl model config file and calls the best_pnl_model with appropriate arguments
#
#    config_file: str
#                The full path of the config file
##
#   using_grid: boolean
#                Whether of use grid or not
    '''

    config_file = args.configfile
    using_grid = args.using_grid
    update_mode = args.updatemode
    lstrat = None
    keep_cname = args.keep_cname
    writeAgain = args.overwrite
    backtest = args.backtest != 0

    current_instruction = None
    reg_string = None
    pred_duration = None
    pred_algo = None
    data_filter = None
    td_string = None
    rd_string = None
    num_lookback_days = None
    walk_start_date = None
    start_time = None
    end_time = None
    trigger_string = None
    target_stdev = None
    max_child_shortcodes = 20
    exec_logic = None
    user_matrix = None

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
                    if current_instruction == "REG_STRING":
                        reg_string = line
                    if current_instruction == "RD_STRING":
                        rd_string = line
                    if current_instruction == "DATA_FILTER":
                        data_filter = line
                    if current_instruction == "TD_STRING":
                        td_string = line
                    if current_instruction == "NUM_LOOKBACK_DAYS":
                        num_lookback_days = line
                    if current_instruction == "WALK_START_DATE":
                        walk_start_date = line
                    if current_instruction == "START_TIME":
                        start_time = line
                    if current_instruction == "END_TIME":
                        end_time = line
                    if current_instruction == "WORK_DIR":
                        work_dir = line
                    if current_instruction == "TRIGGER_STRING":
                        trigger_string = line
                    if current_instruction == "TARGET_STDEV":
                        target_stdev = line
                    if current_instruction == "EXEC_LOGIC":
                        exec_logic = line
                    elif current_instruction == "MODEL_WEIGHTS":
                        if user_matrix is None:
                            user_matrix = []
                        user_matrix.append(list(map(int, line.split())))

    except:
        print(config_file + " not readable")
        raise ValueError(config_file + " not readable.")

    if reg_string is None:
        raise ValueError("REG_STRING is required")
    if data_filter is None:
        raise ValueError("DATA_FILTER is required")
    if start_time is None:
        raise ValueError("START_TIME is required")
    if end_time is None:
        raise ValueError("END_TIME is required")
    if td_string is None:
        raise ValueError("TD_STRING is required")
    if rd_string is None:
        raise ValueError("RD_STRING is required")
    if walk_start_date is None:
        raise ValueError("WALK_START_DATE is required")
    if num_lookback_days is None:
        raise ValueError("NUM_LOOKBACK_DAYS is required")
    if work_dir is None:
        raise ValueError("WORK_DIR is required")
    if trigger_string is None:
        raise ValueError("TRIGGER_STRING is required")
    if target_stdev is None:
        raise ValueError("TARGET_STDEV is required")
    if exec_logic is None:
        raise ValueError("EXEC_LOGIC is required")
    ptime, events, trades = td_string.split()
    pred_algo, pred_duration = rd_string.split()

    print("REGRESS_EXEC: " + reg_string)
    print("PRED_DURATION: " + pred_duration)
    print("PRED_ALGO: " + pred_algo)
    print("DATA_FILTER: " + data_filter)
    print("START_TIME: " + start_time)
    print("END_TIME: " + end_time)
    print("DATAGEN_STRING: " + td_string)
    print("DATE: " + walk_start_date)
    print("NUM_LOOKBACK_DAYS: " + num_lookback_days)
    print("WORK_DIR: " + work_dir)
    print("TRIGGER_STRING: " + trigger_string)
    print("TARGET_STDEV: " + target_stdev)

    end_date = datetime.date.today().strftime("%Y%m%d")
    print("End Date :", end_date)

    # update_mode="RegenerateUniverse"

    now = datetime.datetime.now()
    current_date = now.strftime("%Y%m%d")
    dateNow = str(current_date)
    print("Start Iteration ", dateNow, current_date)
    if datetime.datetime.strptime(str(walk_start_date), '%Y%m%d').weekday() == 5:
        walk_start_date = str(calc_next_week_day(walk_start_date, 1))

    # if called on sunday
    if datetime.datetime.strptime(str(walk_start_date), '%Y%m%d').weekday() == 6:
        walk_start_date = str(calc_next_week_day(walk_start_date, 1))

    shortcode = "DI1X"
    exec_logic_parent = "DI1TradingManager-StructuredGeneralTrading-" + exec_logic
    # exec_logic="PriceBasedAggressiveTrading"
    # name = None
    start_time = "BRT_905"
    end_time = "BRT_1540"
    strat_type = "Regular"
    config_type = "7"
    type = "S"
    walk_start_date = walk_start_date
    ddays_string = num_lookback_days
    trigger_string = trigger_string
    td_string = td_string
    rd_string = pred_algo + " " + pred_duration
    rdata_process_string = data_filter
    reg_string = reg_string
    model_process_string = "0 " + target_stdev
    init_model_file = "/media/shared/ephemeral16/diwakar/GE/im_strats/common_param_0.3"
    param_file = "/media/shared/ephemeral16/diwakar/GE/im_strats/common_param_0.3"
    cname = None
    sname = None

    print("Gold")

    is_structured = str(3)
    iFile = CreateIfileArtificially(shortcode, exec_logic_parent, start_time, end_time, strat_type, config_type, type,
                                    walk_start_date, ddays_string, trigger_string, td_string, rd_string,
                                    rdata_process_string, reg_string, model_process_string, init_model_file,
                                    param_file, work_dir, cname, sname, str(max_child_shortcodes), is_structured, user_matrix)

    [isetParent, cfgParent] = AddConfigtoDB(iFile, lstrat, writeAgain, keep_cname, backtest)

    print("Output of DB of Master Config:")
    # outputArray=output.strip().split("\n")
    configName = cfgParent.cname
    stratName = "w_strat7_" + configName[10:]
    print(configName)
    print(stratName)
    configNameList = []
    for shcIndex in range(0, max_child_shortcodes):
        max_child_shortcodes_child = None
        shortcodeChild = shortcode + "_" + str(shcIndex)
        exec_logic_child = exec_logic
        print("ShortCode :", shortcodeChild)
        childConfigName = configName + "_child_" + str(shcIndex)
        childStratName = stratName + "_child_" + str(shcIndex)
        child_config_type = 7
        is_structured_child = str(0)
        iFileChild = CreateIfileArtificially(shortcodeChild, exec_logic_child, start_time, end_time, strat_type, child_config_type, type,
                                             walk_start_date, ddays_string, trigger_string, td_string, rd_string,
                                             rdata_process_string, reg_string, model_process_string, init_model_file,
                                             param_file, work_dir, childConfigName, childStratName, max_child_shortcodes_child, is_structured_child, user_matrix)
        [isetChild, cfgChild] = AddConfigtoDB(iFileChild, lstrat, writeAgain, keep_cname, backtest)
        #outputArray = output.strip().split("\n")
        configNamenew = cfgChild.cname
        stratNamenew = "w_strat7_" + configNamenew[10:]
        print("Child " + str(shcIndex))
        print(configNamenew)
        print(stratNamenew)

    # work_dir = "/spare/local/" + getpass.getuser() + "/WF/" + cfg.shortcode + "/Parent/" + str(
    # int(time.time() * 1000)) + "/"
    os.system("mkdir --parents " + work_dir)
    stdout_file = work_dir + "stdout.txt"
    stderr_file = work_dir + "stderr.txt"
    print(stdout_file)
    print("CONFIG NAME:", cfgParent.cname)
    print("Generate Models :", args.generate_models)
    end_date = datetime.date.today().strftime("%Y%m%d")
    print("Generate Models2 :", args.generate_models)
    if (cfgParent.config_type == 6 or cfgParent.config_type == 7) and isetParent.walk_start_date is not None:
        start_date = str(isetParent.walk_start_date)
        lookback_days = numpy.busday_count((datetime.datetime.strptime(start_date, "%Y%m%d")).strftime("%Y-%m-%d"),
                                           (datetime.datetime.strptime(end_date, "%Y%m%d")).strftime("%Y-%m-%d")) + 1
    else:
        lookback_days = 250
    print("Generate Models3 :", args.generate_models)

    # if not backtest:
    # print("Running Seq2conf Sensitivity Check...")
    # check_seq2conf_sensitivity(cfg, end_date, lookback_days - 220)

    print("Generate Models4 :", args.generate_models)
    # sys.stdout.flush()
    #sys.stdout = open(stdout_file, 'w')
    #sys.stderr = open(stderr_file, 'w')
    print("Generate Models5 :", args.generate_models)
    if args.generate_models > 0:
        print("Computing models/params for " + str(lookback_days))
        compute_strat_for_config_and_date(cfgParent.cname, int(end_date), int(lookback_days), True, work_dir)

    if args.run_simulations > 0:
        print(("Generating results for " + str(lookback_days)))
        run_simulations(isetParent, cfgParent, int(end_date), int(lookback_days), backtest, work_dir)

    return

    '''while dateNow>walk_start_date:
        dateNow = GenerateStratForDate(dateNow, current_date, reg_string, pred_duration, pred_algo, data_filter, start_time,
                             end_time, td_string, walk_start_date, num_lookback_days, work_dir, trigger_string,
                             target_stdev,update_mode)'''


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', dest='configfile', help="the pnl modelling config", type=str, required=True)
    parser.add_argument('-u', dest='updatemode', help="Regenerate[Universe/Models/Params/All", type=str, required=True)
    parser.add_argument('-r', help='overwrite config', default=0, type=int, required=False, dest='overwrite')
    parser.add_argument('-g', help='generate models', default=0, type=int, required=False, dest='generate_models')
    parser.add_argument('-s', help='run sim', default=0, type=int, required=False, dest='run_simulations')
    parser.add_argument('-a', help='generate type 6 analysis email', type=int, required=False, default=1,
                        dest='type_6_analysis')
    parser.add_argument('--backtest', help='backtest', default=0, type=int, required=False, dest='backtest')
    parser.add_argument('-keepcname', help='preserve cname for lstrat',
                        required=False, dest='keep_cname', default=0, type=int)
    

    parser.add_argument('--nogrid', dest='using_grid',
                        help='whether to use grid or not', default=True,
                        required=False, action='store_false')

    args = parser.parse_args()

    backtest = args.backtest != 0

    if 'USE_BACKTEST' in os.environ:
        backtest = 1
    if backtest:
        print("BACKTEST DB IS SELECTED. CONFIGS, STRATS and RESULTS will goto BACKTEST DB. "
              "CONFIG WILL NOT BE STORED IN FILESYSTEM. NOTE THE CONFIG NAME. BE CAREFUL")
        set_backtest(backtest)
        os.environ['USE_BACKTEST'] = "1"

    # print(args)
    '''if args.using_grid and ('GRID_USERNAME' not in os.environ or os.environ['GRID_USERNAME'] == "" or 'GRID_PASSWORD' not in os.environ or os.environ['GRID_PASSWORD'] == ""):
        grid_user = input("Enter username: ")
        if grid_user == "":
            print("Enter valid username")
            sys.exit(1)
        password = getpass.getpass("Enter password: ")
        os.environ['GRID_USERNAME'] = grid_user
        os.environ['GRID_PASSWORD'] = password'''
    # This command reads the params from the config
    # and also finds the best pnl from the model.
    read_params_from_config(args)
