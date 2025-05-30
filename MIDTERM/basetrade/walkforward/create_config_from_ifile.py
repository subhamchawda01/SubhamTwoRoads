#!/usr/bin/env python

from __future__ import print_function
import argparse
import subprocess
import sys
import os
import time
import json
import datetime
import numpy
import getpass
import random

sys.path.append(os.path.expanduser('~/basetrade/'))
sys.path.append(os.path.expanduser('~/grid/'))

from walkforward.definitions import config
from walkforward.definitions import defines
from walkforward.wf_db_utils import dump_config_to_db
from walkforward.utils.date_utils import calc_prev_week_day
from walkforward.utils.file_utils import clean_parent_dir
from walkforward.wf_db_utils.db_handles import set_backtest
from walkforward.compute_strat_for_config_and_date import compute_strat_for_config_and_date
from walkforward.analyse_type6_config import generate_analysis_for_config
from walkforward.definitions import execs
from walkforward.generate_candidate_params import generate_candidate_params
from walkforward.utils.search_exec_or_script import search_script
from walkforward.wf_db_utils.insert_date_in_diff_format import insert_date_in_diff_format
from walkforward.wf_db_utils.modelling_repo_update_from_db import update_modelling_repo
# Using instruction set object this generates config name and strat name based on their config type


def get_config_strat_names(iset):
    common_name = "_".join([str(iset.config_type), defines.exec_logic_dict[iset.exec_logic],
                            iset.shortcode, iset.base_model_name, iset.start_time, iset.end_time])

    if iset.config_type == 3:
        config_name = "w_config" + common_name
        strat_name = "w_strat" + common_name

    elif iset.config_type == 4:
        name_prefix = "_".join([common_name, iset.param_lookback_days])

        config_name = "w_config" + name_prefix
        strat_name = "w_strat" + name_prefix

    elif iset.config_type == 5:
        name_prefix = "_".join([common_name, "_".join(iset.sample_feature), "_".join(
            iset.feature_switch_threshold), iset.feature_lookback_days])

        config_name = "w_config" + name_prefix
        strat_name = "w_strat" + name_prefix

    elif iset.config_type == 6 or iset.config_type == 7:
        strat_name_prefix = "_".join([common_name, iset.walk_start_date, iset.ddays_string, iset.trigger_string,
                                      iset.td_string, iset.rd_string, iset.rdata_process_string, iset.reg_string,
                                      iset.model_process_string])

        config_name = "w_config" + strat_name_prefix
        strat_name = "w_strat" + strat_name_prefix

    param_idx = "pfi_" + str(iset.base_param_name.split("_")[-1:][0])
    config_name += "_" + str(random.randint(1000, 9999)) + "_" + param_idx
    strat_name += "_" + str(random.randint(1000, 9999)) + "_" + param_idx

    if iset.name is not None:
        config_name = config_name + iset.name
        strat_name = strat_name + iset.name

    config_name = config_name.replace(" ", "_")
    strat_name = strat_name.replace(" ", "_")
    return [config_name, strat_name]


class instruction_set():

    def __init__(self, ifile, lstrat_file):
        # common parameters
        self.shortcode = None
        self.exec_logic = None
        self.name = None
        self.start_time = None
        self.end_time = None
        self.strat_type = "Regular"
        self.config_type = None
        self.model_list = []
        self.param_list = []
        self.param_config = None
        self.event_token = "INVALID"
        self.simula_approved = 0
        self.type = "S"
        self.base_model_name = ''
        self.base_param_name = ''

        # type 4 parameters
        self.param_lookback_days = None

        # type 5 parameters
        self.sample_feature = []
        self.feature_switch_threshold = []
        self.feature_lookback_days = None

        # type 6 parameters
        self.walk_start_date = None
        self.ddays_string = None
        self.trigger_string = None
        self.td_string = None
        self.rd_string = None
        self.rdata_process_string = None
        self.reg_string = None
        self.model_process_string = None
        self.sign_check_string = 0
        self.choose_top_strats = None
        self.max_sum = None
        self.num_regimes = None
        self.user_matrix = None
        self.sort_algo = "kCNAPnlSharpeAverage"
        self.regime_indicator = None
        self.three_step = None
        self.skip_days_file = None
        self.regress_exec = None
        self.is_structured = 0
        self.cname = None
        self.sname = None
        self.max_child_shortcodes = 0

        current_instruction = None
        print("Model List Detect : ")
        print(self.model_list)

        if lstrat_file is not None:
            try:
                with open(lstrat_file) as strat_file:
                    for line in strat_file:
                        params_for_config = line.split()

                        self.config_type = 3  # all legacy strats are considered as type 3 configs

                        # taking input from strat file
                        self.shortcode = params_for_config[1]
                        self.exec_logic = params_for_config[2]

                        self.model_list.append(params_for_config[3])
                        self.base_model_name = os.path.basename(params_for_config[3])

                        self.param_list.append(params_for_config[4])
                        self.base_param_name = os.path.basename(params_for_config[4])

                        self.start_time = params_for_config[5]
                        self.end_time = params_for_config[6]

                        if len(params_for_config) > 8 and params_for_config[8] is not None and params_for_config[8] != "INVALID":
                            self.event_token = params_for_config[8]
                            self.strat_type = 'EBT'

                        if self.exec_logic == 'MeanRevertingTrading' or self.exec_logic == 'IndexFuturesMeanRevertingTrading':
                            self.strat_type = 'MRT'

            except:
                print(lstrat_file + " not readable !")
                raise ValueError(ifile + "not a valid regular instruction file.")

        else:
            try:
                with open(ifile) as file:
                    for line in file:
                        line = line.strip()
                        if len(line) == 0:
                            current_instruction = None
                            continue

                        if line[0][0] == "#":
                            continue

                        if current_instruction is None:
                            current_instruction = line

                        else:
                            # checks for common parameters
                            if current_instruction == "SHORTCODE":
                                self.shortcode = line
                            elif current_instruction == "EXEC_LOGIC":
                                self.exec_logic = line
                            elif current_instruction == "NAME":
                                self.name = "_" + line
                            elif current_instruction == "START_TIME":
                                self.start_time = line
                            elif current_instruction == "END_TIME":
                                self.end_time = line
                            elif current_instruction == "STRAT_TYPE":
                                self.strat_type = line
                            elif current_instruction == "EVENT_TOKEN":
                                self.event_token = line
                            elif current_instruction == "CONFIG_TYPE":
                                self.config_type = int(line)
                            elif current_instruction in ["MODEL_LIST", "MODEL_FILE", "INIT_MODEL_LIST", "INIT_MODEL_FILE"]:
                                self.model_list.append(line)
                                if self.base_model_name == '':
                                    self.base_model_name = os.path.basename(line)
                            elif current_instruction in ["PARAM_LIST", "PARAM_FILE"]:
                                self.param_list.append(line)
                                if self.base_param_name == '':
                                    self.base_param_name = os.path.basename(line)
                            elif current_instruction == "PARAM_CONFIG":
                                self.param_config = line

                            # checks for type 4 parameters
                            elif current_instruction == "PARAM_LOOKBACK_DAYS":
                                self.param_lookback_days = line

                            # checks for type 5 parameters
                            elif current_instruction == "SAMPLE_FEATURE":
                                self.sample_feature = line.split()
                            elif current_instruction == "FEATURE_SWITCH_THRESHOLD":
                                self.feature_switch_threshold = line.split()
                            elif current_instruction == "FEATURE_LOOKBACK_DAYS":
                                self.feature_lookback_days = line

                            # checks for type 6 config-column parameters
                            elif current_instruction == "WALK_START_DATE":
                                self.walk_start_date = line
                            elif current_instruction == "DDAYS_STRING":
                                self.ddays_string = line
                            elif current_instruction == "TRIGGER_STRING":
                                self.trigger_string = line
                            elif current_instruction == "TD_STRING":
                                self.td_string = line
                            elif current_instruction == "RD_STRING":
                                self.rd_string = line
                            elif current_instruction == "RDATA_PROCESS_STRING":
                                self.rdata_process_string = line
                            elif current_instruction == "REG_STRING":
                                self.reg_string = line
                                self.regress_exec = line.split()[0]
                            elif current_instruction == "MODEL_PROCESS_STRING":
                                self.model_process_string = line

                            # type 6 json parameters (for model training)
                            elif current_instruction == "SIGN_CHECK":
                                self.sign_check_string = line
                            elif current_instruction == "CHOOSE_TOP_STRATS":
                                self.choose_top_strats = line
                            elif current_instruction == "MAX_SUM_INDICATORS":
                                self.max_sum = line
                            elif current_instruction == "NUM_REGIMES":
                                self.num_regimes = line
                            elif current_instruction == "MODEL_WEIGHTS":
                                if self.user_matrix is None:
                                    self.user_matrix = []
                                self.user_matrix.append(list(map(int, line.split())))
                            elif current_instruction == "SORT_ALGO":
                                self.sort_algo = line
                            elif current_instruction == "REGIME_INDICATOR":
                                self.regime_indicator = line
                            elif current_instruction == "THREE_STEP_OPTIM":
                                self.three_step = line
                            elif current_instruction == "SKIP_DAYS":
                                self.skip_days_file = line
                            elif current_instruction == "IS_STRUCTURED":
                                self.is_structured = int(line)
                            elif current_instruction == "CNAME":
                                self.cname = line
                            elif current_instruction == "SNAME":
                                self.sname = line
                            elif current_instruction == "MAX_CHILD_SHORTCODES":
                                self.max_child_shortcodes = int(line)

            except:
                print(ifile + " not readable !")
                raise ValueError(ifile + "not a valid regular instruction file.")

            if self.config_type == 6 or self.config_type == 7:
                if self.regress_exec == "PNL":
                    self.rd_string = "0 0"
                    self.td_string = "10000 0 0"
                    self.rdata_process_string = "f0"
                    if self.three_step is None:
                        self.three_step = 0
                    elif self.three_step != 0:
                        self.three_step = 1

            if self.event_token != "INVALID":
                self.strat_type = 'EBT'

            if self.exec_logic == 'MeanRevertingTrading' or self.exec_logic == 'IndexFuturesMeanRevertingTrading':
                self.strat_type = 'MRT'


def get_common_fields_map(iset, description, query_id=1001, simula_approved=0, type='S'):
    """
    returns common keys across all config type
    :param iset: 
    :param description: 
    :param query_id: 
    :param simula_approved: 
    :param type: 
    :return: 
    """
    common_fields = {"shortcode": iset.shortcode,
                     "execlogic": iset.exec_logic,
                     "start_time": iset.start_time,
                     "end_time": iset.end_time,
                     "strat_type": iset.strat_type,
                     "event_token": iset.event_token,
                     "config_type": iset.config_type,
                     "model_list": iset.model_list,
                     "param_list": iset.param_list,
                     "is_structured": iset.is_structured,
                     "query_id": query_id,
                     "simula_approved": simula_approved,  # this one is missing from constructor
                     "type": type,  # this one is missing from constructor
                     "description:": description  # this one is missing from constructor
                     }
    return common_fields


def check_fields_validity(iset, common_fields, type_specific_field):
    """
    checks for field/keys validity of the iset based on its config type
    :param iset: 
    :param common_fields: 
    :param type_specific_field: 
    :return: 
    """
    type4_keys = ['param_lookback_days']
    type5_keys = ['sample_feature', 'feature_switch_threshold', 'feature_lookback_days']
    type6_keys = ['walk_start_date', 'ddays_string', 'trigger_string', 'reg_string', 'model_process_string']
    type6_no_pnl_keys = ['td_string', 'rd_string', 'rdata_process_string']

    for (k, v) in common_fields.items():
        if v is None:
            raise ValueError(k + " is required in config")

    if iset.config_type == 4:
        for k in type4_keys:
            if k not in type_specific_field.keys() or type_specific_field[k] is None:
                raise ValueError(k + " is required in type 4 config")

    if iset.config_type == 5:
        for k in type5_keys:
            if k not in type_specific_field.keys() or type_specific_field[k] is None:
                raise ValueError(k + " is required in type 5 config")

    if iset.config_type == 6:
        for k in type6_keys:
            if k not in type_specific_field.keys() or type_specific_field[k] is None:
                raise ValueError(k + " is required in type 6 config")
        if iset.regress_exec != "PNL":
            for k in type6_no_pnl_keys:
                if k not in type_specific_field.keys() or type_specific_field[k] is None:
                    raise ValueError(k + " is required in type 6 config without PNL modelling")
        else:
            if iset.choose_top_strats is None:
                raise ValueError("CHOOSE_TOP_STRATS is required for PNL modelling")
            if iset.max_sum is None and len(iset.user_matrix) == 0 and iset.three_step is None:
                raise ValueError("Either of MAX_SUM or MODEL_WEIGHTS or THREE_STEP_OPTIM is required for PNL modelling")
            if iset.regime_indicator is not None and iset.num_regimes is None:
                raise ValueError("REGIME_INDICATOR specified but NUM_REGIMES not specified for PNL modelling")
            if iset.regime_indicator is None and iset.num_regimes is not None:
                raise ValueError("NUM_REGIMES specified but REGIME_INDICATOR not specified for PNL modelling")
        if iset.param_config != None:
            if iset.model_process_string == None or iset.model_process_string == 0 or iset.model_process_string == '0':
                raise ValueError("PARAM_CONFIG specified but Target Stdev not specified in MODEL_PROCESS_STRING")


def get_type_specific_fields(iset):
    """
    returns config type specific fields   
    :param iset: 
    :return: 
    """
    type_specific_field = {}
    if iset.config_type == 4:
        type_specific_field = {'param_lookback_days': iset.param_lookback_days}
        if iset.sort_algo is not None:
            type_specific_field['sort_algo'] = iset.sort_algo

    elif iset.config_type == 5:
        type_specific_field = {'sample_feature': iset.sample_feature,
                               'feature_switch_threshold': iset.feature_switch_threshold,
                               'feature_lookback_days': iset.feature_lookback_days}
    elif iset.config_type == 6 or iset.config_type == 7:
        type_specific_field = {'walk_start_date': iset.walk_start_date,
                               'ddays_string': iset.ddays_string,
                               'trigger_string': iset.trigger_string,
                               'td_string': iset.td_string,
                               'rd_string': iset.rd_string,
                               'rdata_process_string': iset.rdata_process_string,
                               'reg_string': iset.reg_string,
                               'model_process_string': iset.model_process_string,
                               "sign_check": iset.sign_check_string,
                               "max_sum": iset.max_sum,
                               "user_matrix": iset.user_matrix,
                               "sort_algo": iset.sort_algo,
                               "three_step_optim": iset.three_step,
                               "choose_top_strats": iset.choose_top_strats,
                               "skip_days": iset.skip_days_file,
                               "regime_indicator": iset.regime_indicator,
                               "num_regimes": iset.num_regimes,
                               "param_config": iset.param_config,
                               "cname": iset.cname,
                               "sname": iset.sname,
                               "max_child_shortcodes": iset.max_child_shortcodes,
                               "volume_range": [],
                               "ttc_range": [],
                               "default_param": []

                               }

    return type_specific_field


def create_config(iset, overwrite, lstrat, keep_cname, backtest, pnl_modelling_strat, given_cname):
    """
    creates config object using json object  
    :param args: 
    :return: 
    """
    #iset = instruction_set(args.ifile, args.lstrat)
    #overwrite = args.overwrite != 0
    # common value checks
    if iset.config_type is None:
        raise ValueError("config type is not defined")

    description = 'This config is created on ' + datetime.date.today().isoformat()
    common_fields = get_common_fields_map(iset, description)
    type_specific_field = get_type_specific_fields(iset)

    check_fields_validity(iset, common_fields, type_specific_field)

    if iset.config_type != 7 or (iset.cname == None or iset.sname == None):
        cname, sname = get_config_strat_names(iset)
        if given_cname != "":
            cname = given_cname
    else:
        cname = iset.cname
        sname = iset.sname

    # Handle for param_config
    if iset.config_type == 6:
        if iset.param_config != None:
            min_price_increment_cmd = [execs.execs().get_min_price_increment, iset.shortcode,
                                       str(time.strftime("%Y%m%d"))]
            process = subprocess.Popen(' '.join(min_price_increment_cmd), shell=True,
                                       stderr=subprocess.PIPE,
                                       stdout=subprocess.PIPE)
            out, err = process.communicate()
            if out is not None:
                out = out.decode('utf-8')
            if err is not None:
                err = err.decode('utf-8')

            product_min_price_increment = float(out)
            model_stdev_ticks = float(iset.model_process_string.split()[1]) / product_min_price_increment
            generate_candidate_params_output = generate_candidate_params(iset.param_config, model_stdev_ticks)
            # Making param_list as default param file (present in the param config file)
            common_fields["param_list"] = [generate_candidate_params_output[3]]
            # Defining a default param in json just to distinguish which param was provided as default and which was dynamically generated.
            type_specific_field["default_param"] = common_fields["param_list"].copy()
            # Appending dynamic generated params to param list. Keeping default as initial param with index = 0
            common_fields["param_list"].extend(generate_candidate_params_output[0])
            # Get the volume range check
            type_specific_field["volume_range"] = generate_candidate_params_output[1]
            # Get the ttc range check
            type_specific_field["ttc_range"] = generate_candidate_params_output[2]

    all_fields = dict(list(common_fields.items()) + list(type_specific_field.items()))

    all_fields["sname"] = sname

    if lstrat is not None and keep_cname == 1 and iset.is_structured <= 1:
        all_fields["cname"] = os.path.basename(lstrat)
    else:
        all_fields["cname"] = cname

        # we first make the json object that dictate config logic to create strategy per date
    # apparently diwakar thinks its cool to have all these fields in json object as well, so we append this to actual json

    # we are creating config ( yes it is a class ) object that will be passed
    typenconfig = config.config.initialize()
    typenconfig.config_json = json.dumps(all_fields, sort_keys=True)

    print(typenconfig.config_json)
    typenconfig.update_config_from_its_json()
    typenconfig.add_cname(all_fields["cname"])
    typenconfig.add_sname(all_fields["sname"])
    typenconfig.add_simula_approved(iset.simula_approved)
    typenconfig.add_type(iset.type)

    (exists, configid) = dump_config_to_db.dump_config_to_db(typenconfig, overwrite)
    typenconfig.configid = configid

    if lstrat is not None and pnl_modelling_strat:
        pnl_modelling_dir = os.path.dirname(os.path.dirname(lstrat))

        training_days_file = os.path.join(pnl_modelling_dir, "training_days_file")
        f = open(training_days_file, "r")
        training_days_list = f.read().splitlines()
        f.close()

        validation_days_file = os.path.join(pnl_modelling_dir, "validation_days_file")
        f = open(validation_days_file, "r")
        validation_days_list = f.read().splitlines()
        f.close()

        testing_days_file = os.path.join(pnl_modelling_dir, "testing_days_file")
        f = open(testing_days_file, "r")
        testing_days_list = f.read().splitlines()
        f.close()

        preprocessing_days_list = []
        preprocessing_days_file = os.path.join(pnl_modelling_dir, "preprocessing_days")
        if (os.path.exists(preprocessing_days_file)):
            f = open(preprocessing_days_file, "r")
            preprocessing_days_list = f.read().splitlines()
            f.close()

        insert_date_in_diff_format(configid, training_days_list, validation_days_list, testing_days_list,
                                   preprocessing_days_list)

    if exists == -1:
        print('no configname specified')
        sys.exit()
    if exists == 0 and not overwrite:
        print("config already present. not overwriting.. nothing to do")
        sys.exit()

    if not backtest:
        # dump_config_to_db.dump_config_to_fs(typenconfig)
        update_modelling_repo(typenconfig.cname)

    return [iset, typenconfig]


def check_seq2conf_sensitivity(cfg, end_date, lookback_days):

    file_with_config_name = work_dir + cfg.cname + "_cfile"
    with open(os.path.join(work_dir, file_with_config_name), 'w+') as file:
        print(("opened file to write " + file_with_config_name))
        file.write(cfg.cname)

    dates_cmd = [execs.execs().get_dates, cfg.shortcode, str(lookback_days), str(end_date)]
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

    all_dates = out.split()

    run_sim_dates = [dt for dt in all_dates]
    start_date = min(run_sim_dates)

    run_sim_dates_file = os.path.join(work_dir, "run_sim_dates")
    w = open(run_sim_dates_file, 'w')
    for dt in run_sim_dates:
        w.write(dt + "\n")
    w.close()
    seq2conf_sensitivity_check_cmd = [search_script("seq2conf_sensitivity_check.py"), "-shc", cfg.shortcode, "-configdir", file_with_config_name, "-sdate", start_date,
                                      "-edate", end_date, "-send_email", "1"]

    print(("Seq2conf_sensitivity_check CMD " + ' '.join(seq2conf_sensitivity_check_cmd)))
    os.system(' '.join(seq2conf_sensitivity_check_cmd))

    print("Have run the seq2conf check; lookout for the mail\n")


def run_simulations(iset, cfg, end_date, lookback_days, backtest, work_dir):
    """

    :param iset:
    :param cfg:
    :param end_date:
    :param lookback_days:
    :param backtest:
    :return:
    """
    file_with_config_name = work_dir + cfg.cname + "_cfile"
    with open(os.path.join(work_dir, file_with_config_name), 'w+') as file:
        print(("opened file to write " + file_with_config_name))
        file.write(cfg.cname)

    shortcode = ""
    if cfg.is_structured != 3:
        shortcode = cfg.shortcode
    else:
        shortcode = "ALL"

    dates_cmd = [execs.execs().get_dates, shortcode, str(lookback_days), str(end_date)]
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

    all_dates = out.split()

    skip_dates = []
    if iset.skip_days_file is not None and iset.skip_days_file != "":
        with open(iset.skip_days_file) as f:
            skip_dates = f.read().splitlines()

    run_sim_dates = [dt for dt in all_dates if dt not in skip_dates]
    start_date = min(run_sim_dates)

    run_sim_dates_file = os.path.join(work_dir, "run_sim_dates")
    w = open(run_sim_dates_file, 'w')
    for dt in run_sim_dates:
        w.write(dt + "\n")
    w.close()

    if cfg.is_structured != 3:
        run_simulations_cmd = [execs.execs().run_simulations, shortcode, file_with_config_name, str(start_date),
                               str(end_date), "DB", '-r', '1', '-dtlist', run_sim_dates_file]
    else:
        run_simulations_cmd = [execs.execs().run_simulations, shortcode, file_with_config_name, str(start_date),
                               str(end_date), "DB", '-r', '1', '-dtlist', run_sim_dates_file, "-st 1", "--nogrid"]
    if backtest:
        run_simulations_cmd += ['--backtest']
    print(("RUN_SIMULATIONS_CMD " + ' '.join(run_simulations_cmd)))

    out = subprocess.Popen(' '.join(run_simulations_cmd), shell=True, stdout=subprocess.PIPE)
    run_simulations_output = out.communicate()[0].decode('utf-8').strip()
    print(("RUN_SIM_OUTPUT " + run_simulations_output))


if __name__ == "__main__":

    # command line arguements
    parser = argparse.ArgumentParser(
        description="creates a new config in DB using instruction file or legacy-strat file")
    parser.add_argument('-ifile', help='instruction file', required=False, dest='ifile')
    parser.add_argument('-lstrat', help='legacy strat file', required=False, dest='lstrat')
    parser.add_argument('-r', help='overwrite config', default=0, type=int, required=False, dest='overwrite')
    parser.add_argument('-g', help='generate models', default=0, type=int, required=False, dest='generate_models')
    parser.add_argument('-s', help='run sim', default=0, type=int, required=False, dest='run_simulations')
    parser.add_argument('-a', help='generate type 6 analysis email', type=int,
                        required=False, default=1, dest='type_6_analysis')
    parser.add_argument('-keepcname', help='preserve cname for lstrat',
                        required=False, dest='keep_cname', default=0, type=int)
    parser.add_argument('--backtest', help='backtest', default=0, type=int, required=False, dest='backtest')
    parser.add_argument('--pnl_modelling_strat', help='Is it a pnl_modelling strat', default=0,
                        type=int, required=False, dest='pnl_modelling_strat')
    parser.add_argument('--cname', help='cname for the given lstrat', default="",
                        type=str, required=False, dest='cname')

    args = parser.parse_args()

    type_6_analysis_flag = args.type_6_analysis

    if 'GRID_USERNAME' not in os.environ or os.environ['GRID_USERNAME'] == "" or 'GRID_PASSWORD' not in os.environ or os.environ['GRID_PASSWORD'] == "":
        grid_user = input("Enter grid username: ")
        if grid_user == "":
            print("Enter valid username")
            sys.exit(1)
        password = getpass.getpass("Enter grid password: ")
        os.environ['GRID_USERNAME'] = grid_user
        os.environ['GRID_PASSWORD'] = password

    if args.ifile is None and args.lstrat is None:
        parser.error("at least one of ifile or lstrat required to create config")

    elif args.ifile is not None and args.lstrat is not None:
        parser.error("at most one of ifile or lstrat can be provided")

    backtest = args.backtest != 0

    if 'USE_BACKTEST' in os.environ:
        backtest = 1
    if backtest:
        print("BACKTEST DB IS SELECTED. CONFIGS, STRATS and RESULTS will goto BACKTEST DB. "
              "CONFIG WILL NOT BE STORED IN FILESYSTEM. NOTE THE CONFIG NAME. BE CAREFUL")
        set_backtest(backtest)
        os.environ['USE_BACKTEST'] = "1"

    iset = instruction_set(args.ifile, args.lstrat)
    overwrite = args.overwrite != 0
    lstrat = args.lstrat
    keep_cname = args.keep_cname
    given_cname = args.cname
    pnl_modelling_strat = args.pnl_modelling_strat

    [iset, cfg] = create_config(iset, overwrite, lstrat, keep_cname, backtest, pnl_modelling_strat, given_cname)

    work_dir = "/spare/local/" + getpass.getuser() + "/WF/" + cfg.shortcode + "/Parent/" + str(int(time.time() * 1000)) + "/"
    os.system("mkdir --parents " + work_dir)
    stdout_file = work_dir + "stdout.txt"
    stderr_file = work_dir + "stderr.txt"
    print(stdout_file)
    print("CONFIG NAME:", cfg.cname)

    end_date = datetime.date.today().strftime("%Y%m%d")

    if (cfg.config_type == 6 or cfg.config_type == 7) and iset.walk_start_date is not None:
        start_date = str(iset.walk_start_date)
        lookback_days = numpy.busday_count((datetime.datetime.strptime(start_date, "%Y%m%d")).strftime("%Y-%m-%d"),
                                           (datetime.datetime.strptime(end_date, "%Y%m%d")).strftime("%Y-%m-%d")) + 1
    else:
        lookback_days = 1000

    sys.stdout.flush()
    sys.stdout = open(stdout_file, 'w')
    sys.stderr = open(stderr_file, 'w')

    if args.generate_models > 0:
        print(("Computing models/params for " + str(lookback_days)))
        try:
            compute_strat_for_config_and_date(cfg.cname, int(end_date), int(lookback_days), True, work_dir)
        except Exception as e:
            clean_parent_dir(work_dir)
            print("Error in compute_strat_for_config " + str(e) + "\n")
            sys.exit()

    if args.run_simulations > 0:
        print(("Generating results for " + str(lookback_days)))
        run_simulations(iset, cfg, end_date, lookback_days, backtest, work_dir)

    if cfg.config_type == 6 or cfg.config_type == 7:
        if args.generate_models == 0 or args.run_simulations == 0:
            print("Type 6 analyse flag set to 0 as either generate_models or run_simulation flag is set to 0.")
            type_6_analysis_flag = 0
    else:
        type_6_analysis_flag = 0

    if type_6_analysis_flag > 0:
        print("Generating email of type 6 analysis")
        generate_analysis_for_config(cfg.cname, 'nseall@tworoads.co.in', 1)

    if not backtest:
        print("Running Seq2conf Sensitivity Check...")
        check_seq2conf_sensitivity(cfg, end_date, 30)

    clean_parent_dir(work_dir)
