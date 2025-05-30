#!/usr/bin/python
"""
 This script is a helper script that helps to analyse the sensitivity of a strat or a list of strat wrt seq2conf value
 This uses run_simulations underneath to produce results in resdir which can be used to see the change in PNL cause due to different seq2conf
 A pair of seq2conf multiplier and addend is read from  sensitivity_param_file_ and
 the new seq2conf delay is defined as original seq2conf_delay*seq2conf_multiplier + seq2conf_addend
"""

import argparse
import sys
import os
from os import makedirs
import pandas as pd
import subprocess
import shutil
import getpass
from shutil import copyfile
import fileinput
sys.path.append(os.path.expanduser("~/basetrade"))
from walkforward.definitions import execs

""" copying sim config """
def copy_sim_config(sim_config_dir, current_user_):
    sim_config_src_ = os.path.join("/home/", current_user_, sim_config_dir, "sim_config.txt")
    sim_config_dst_ = os.path.join(os.environ["DEPS_INSTALL"], sim_config_dir, "sim_config.txt")
    copyfile(sim_config_src_, sim_config_dst_)


""" copying market model info """
def copy_market_model_info(current_user_):
    base_path_market_model_info_ = execs.execs().market_model_info_base
    market_model_info_dir_ = os.path.join(os.environ["DEPS_INSTALL"], base_path_market_model_info_)
    makedirs(market_model_info_dir_, 0o777, exist_ok=True, )

    base_path_market_model_info_file_ = os.path.join(execs.execs().market_model_info_base, "market_model_info.txt")
    market_model_info_src_ = os.path.join("/home/", current_user_, base_path_market_model_info_file_)
    market_model_info_dst_ = os.path.join(os.environ["DEPS_INSTALL"], base_path_market_model_info_file_)
    copyfile(market_model_info_src_, market_model_info_dst_)


""" inserts(inplace) multiplier and addend into temp sim_config.txt which is read by run_simulations """
def insert_multiplier_addend_in_sim_config(sim_config_dst_, shortcode_, multiplier_, addend_):
    for line in fileinput.input(sim_config_dst_, inplace=1):
        line = line.rstrip()
        print(line)
        if shortcode_ in line:
            print(str("SIMCONFIG USE_SEQ2CONF_MULTIPLIER " + multiplier_))
            print(str("SIMCONFIG USE_SEQ2CONF_ADDEND " + addend_))


"""  command line arguments """
parser = argparse.ArgumentParser(description="checks the sensivity of strats wrt seq2conf delays")
parser.add_argument('-shc', help='shortcode', required=True, dest='shc_')
parser.add_argument('-sdir', help='strat_directory', required=True, dest='strat_dir_')
parser.add_argument('-sdate', help='start date', required=True, dest='start_date_')
parser.add_argument('-edate', help='end date', required=True, dest='end_date_')
parser.add_argument('-resdir', help='result dir', required=True, dest='res_dir_')
parser.add_argument('-spf', help='sensitivity parameter file', required=True, dest='sensitivity_param_file_')
args = parser.parse_args()

sim_config_dir = execs.execs().sim_config_base
shortcode_ = args.shc_
strat_dir_ = args.strat_dir_
start_date_ = args.start_date_
end_date_ = args.end_date_
results_dir_ = args.res_dir_
sensitivity_param_file_ = args.sensitivity_param_file_
temp_sim_config_dir_ = args.sensitivity_param_file_

current_user_ = getpass.getuser()

""" setting environment variable """
os.environ["WORKDIR"] = "tmp"
os.environ["DEPS_INSTALL"] = os.path.join("/home/", current_user_, "t_" )

""" creating directory """
dir_ = os.path.join(os.environ["DEPS_INSTALL"], sim_config_dir)
makedirs(dir_, 0o777, exist_ok=True)
makedirs(results_dir_, 0o777, exist_ok=True, )

"""  file path """
sim_config_file_path = os.path.join(dir_, "sim_config.txt")

""" reading user inputted sensitivity params """
sensitivity_params_ = pd.read_csv(sensitivity_param_file_, sep=",")

copy_sim_config(sim_config_dir, current_user_)
copy_market_model_info(current_user_)

base_path_market_model_info_file_ = "baseinfra/OfflineConfigs/MarketModelInfo/market_model_info.txt"
market_model_info_dst_ = os.path.join(os.environ["DEPS_INSTALL"], base_path_market_model_info_file_)
sim_config_dst_ = os.path.join(os.environ["DEPS_INSTALL"], sim_config_dir, "sim_config.txt")

for index, sp_ in sensitivity_params_.iterrows():
    copy_market_model_info(current_user_)
    copy_sim_config(sim_config_dir, current_user_)
    insert_multiplier_addend_in_sim_config(sim_config_dst_, shortcode_, str(sp_["Multiplier"]), str(sp_["Addend"]))

    current_result_directory_ = os.path.join(results_dir_, str(sp_["Multiplier"]), str(sp_["Addend"]))
    print("RUNNING FOR SEQ DELAY MULTIPLIER : " + str(sp_["Multiplier"]) + "    DELAY ADDEND : " + str(sp_["Addend"]))

    makedirs(current_result_directory_, 0o777, exist_ok=True, )
    run_simulations_cmd_ = [execs.execs().run_simulations, shortcode_, strat_dir_, start_date_, end_date_,
                            current_result_directory_, "-d", "0", "--nogrid"]
    print(("RUN_SIMULATIONS_CMD " + ' '.join(run_simulations_cmd_)))
    out = subprocess.Popen(' '.join(run_simulations_cmd_), shell=True, stdout=subprocess.PIPE)
    run_simulations_output = out.communicate()[0].decode('utf-8').strip()

    print(("RUN_SIM_OUTPUT " + run_simulations_output))
    print("\n+++================================================================================+++\n")

""" removing the dir"""
shutil.rmtree(os.environ["DEPS_INSTALL"])

""" removing env variables """
del os.environ["WORKDIR"]
del os.environ["DEPS_INSTALL"]
