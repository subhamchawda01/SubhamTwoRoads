#!/usr/bin/env python

"""
# \file walkforward/generate_candidate_params.py
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#


# INPUT
# ModelStdev
# Config

# OUTPUT
# Param sets based on different threshold profiles

# Methodology
# For now just hardcoding the following logic for coming up with the params.
# place_keep_diff = 0.2 * zeropos_keep;  0.2 is place_keep_diff_factor
# decrease_keep = zeropos_keep * decrease_factor;
# increase_keep = zeropos_keep * increase_factor;
# 1 - decrease_factor  = 0.5 * (increase_factor -1);  0.5 (= decrease_increase_ratio) signifies is reduction in threshold for decrease compared to increment in threshold for increase
# zeropos_keep = thresh_factor * model_stdev;
# Effectively two params : thresh_factor, increase_factor
# thresh_factor = ( 1.8, 2.4, 3.0 )
# increase_factor = ( 1.0, 1.4, 2.0 )
# results in 9 params

# TODO
# Add a support for config which dictates how to come up with candidate params
# incorporating more nuances of params like whether we should have linear scaling or exponential
# ( currently achieved by using highpos params ) or better ways of deciding MUR or aggress thresholds

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

from walkforward.wf_db_utils import dump_strat_for_config_for_day


def signal_handler(signal, frame):
    print("There is a siginit")
    sys.exit(0)


signal.signal(signal.SIGINT, signal_handler)

class pconfig():

    param_file = None
    thresh_factor_list = []
    increase_factor_list = []
    decrease_increase_ratio = 0.0
    current_instruction = None
    place_keep_diff_factor = 0.0
    volume_range = []
    ttc_range = []

    def __init__(self,param_config_):
        current_instruction = None
        with open(param_config_) as file:
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
                    if current_instruction == "THRESH_FACTOR":
                        self.thresh_factor_list.append(float(line.split()[0]))
                    elif current_instruction == "INCREASE_FACTOR":
                        self.increase_factor_list.append(float(line.split()[0]))
                    elif current_instruction == "DEFAULT_PARAM_FILE":
                        self.param_file = line.split()[0]
                    elif current_instruction == "DECREASE_INCREASE_RATIO":
                        self.decrease_increase_ratio = float(line.split()[0])
                    elif current_instruction == "PLACE_KEEP_DIFF_FACTOR":
                        self.place_keep_diff_factor = float(line.split()[0])
                    elif current_instruction == "VOLUME_RANGE":
                        self.volume_range = list(map(float,line.split()))
                    elif current_instruction == "TTC_RANGE":
                        self.ttc_range = list(map(float,line.split()))


def generate_candidate_params(param_config_, model_stdev_, parent_work_dir_=None):
    """

    :param param_config_:
    :param model_stdev_:
    :param product_stdev_:
    :param parent_work_dir_:
    :return:
    """

    if parent_work_dir_ == None:
        parent_work_dir_ = "/media/shared/ephemeral16/" + getpass.getuser() + "/candidate_params/"  + str(int(time.time() * 1000)) + "/"

    os.system("mkdir --parents " + parent_work_dir_)

    param_file_list_ = []
    pconfig_ = pconfig(param_config_)

    max_unit_ratio_ = 1
    with open(pconfig_.param_file, mode="r") as file:
        for line in file:
            line = line.strip()
            line = line.split()
            if len(line) == 0:
                continue

            if line[0][0] == "#":
                continue

            if line[1] == "MAX_UNIT_RATIO":
                max_unit_ratio_ = int(line[2])

    pcount_ = 0

    for thresh_factor_ in pconfig_.thresh_factor_list :
        for increase_factor_ in pconfig_.increase_factor_list:
            zeropos_keep_ = model_stdev_ * thresh_factor_
            place_keep_diff_ = zeropos_keep_ * pconfig_.place_keep_diff_factor
            increase_keep_ = zeropos_keep_ * increase_factor_
            increase_zeropos_diff_ = increase_keep_ - zeropos_keep_
            decrease_factor_ = 1 - (increase_factor_ - 1) * pconfig_.decrease_increase_ratio
            decrease_keep_ = zeropos_keep_ * decrease_factor_
            zeropos_decrease_diff_ = zeropos_keep_ - decrease_keep_
            new_param_file_ = parent_work_dir_ + "/param_file_" + str(pcount_)
            param_file_list_.append(new_param_file_)
            pcount_+=1

            os.system("cp " + str(pconfig_.param_file) + " " + str(new_param_file_))
            os.system("sed -i '/ZEROPOS_KEEP/d' " + str(new_param_file_))
            os.system("sed -i '/ZEROPOS_PLACE/d' " + str(new_param_file_))
            os.system("sed -i '/ZEROPOS_DECREASE_DIFF/d' " + str(new_param_file_))
            os.system("sed -i '/INCREASE_ZEROPOS_DIFF/d' " + str(new_param_file_))
            os.system("sed -i '/PLACE_KEEP_DIFF/d' " + str(new_param_file_))
            os.system("sed -i '/INCREASE_PLACE/d' " + str(new_param_file_))
            os.system("sed -i '/INCREASE_KEEP/d' " + str(new_param_file_))
            os.system("sed -i '/DECREASE_PLACE/d' " + str(new_param_file_))
            os.system("sed -i '/DECREASE_KEEP/d' " + str(new_param_file_))
            os.system("sed -i '/HIGHPOS_SIZE_FACTOR/d' " + str(new_param_file_))
            os.system("sed -i '/HIGHPOS_THRESH_DECREASE/d' " + str(new_param_file_))
            os.system("sed -i '/HIGHPOS_THRESH_FACTOR/d' " + str(new_param_file_))
            os.system("sed -i '/ZEROPOS_LIMITS_UNIT_RATIO/d' " + str(new_param_file_))
            os.system("sed -i '/HIGHPOS_LIMITS_UNIT_RATIO/d' " + str(new_param_file_))

            param_file_ = open(new_param_file_, mode='a')
            param_file_.write("PARAMVALUE ZEROPOS_KEEP " + str(zeropos_keep_))
            param_file_.write("\nPARAMVALUE PLACE_KEEP_DIFF " + str(place_keep_diff_))
            param_file_.write("\nPARAMVALUE INCREASE_ZEROPOS_DIFF " + str(increase_zeropos_diff_))
            param_file_.write("\nPARAMVALUE ZEROPOS_DECREASE_DIFF " + str(zeropos_decrease_diff_))
            param_file_.write("\nPARAMVALUE HIGHPOS_SIZE_FACTOR 0")
            param_file_.write("\nPARAMVALUE HIGHPOS_THRESH_FACTOR 0")
            param_file_.write("\nPARAMVALUE HIGHPOS_THRESH_DECREASE 0")            
            param_file_.write("\nPARAMVALUE HIGHPOS_LIMITS_UNIT_RATIO " + str(max_unit_ratio_))
            param_file_.write("\nPARAMVALUE ZEROPOS_LIMITS_UNIT_RATIO " + str(max_unit_ratio_) + "\n")
            param_file_.close()

    # Reading volume range and ttc range from the param config and returning it ahead.
    return param_file_list_, pconfig_.volume_range, pconfig_.ttc_range, pconfig_.param_file

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="creates candidate param files")
    parser.add_argument('-pconfig', help='param generation config', required=True, dest='pconfig')
    parser.add_argument('-msd', help='model stdev', type=float,required=True, dest='msd')
    parser.add_argument('-work_dir', help='work directory', default=None, required=False, dest='work_dir')
    args = parser.parse_args()

    print(generate_candidate_params(args.pconfig, args.msd, args.work_dir))
