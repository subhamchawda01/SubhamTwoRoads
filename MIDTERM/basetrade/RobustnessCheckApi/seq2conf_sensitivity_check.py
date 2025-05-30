#!/usr/bin/env python

"""
 This script is a tool which helps to gauge the sensitivity of strats in the pool of any product. It lists the strats in
 descending order of its stability wrt variable seq2conf.

 USAGE : ./seq2conf_sensitivity_check shortcode -shc <6A_0> -session <EST_800-EST_1600>  -sdate <20170815> -edate <20170817> -results <~/resdirectory/out>


"""

import argparse
import sys
import os
from itertools import starmap
from os import makedirs
import pandas as pd
import subprocess
import shutil
from shutil import copyfile
import fileinput
sys.path.append(os.path.expanduser("~/basetrade"))
from walkforward.definitions import execs
from random import randint
import getpass
from email.mime.text import MIMEText
from email.mime.image import MIMEImage
from email.mime.multipart import MIMEMultipart

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
def insert_multiplier_addend_in_sim_config(sim_config_dst_, shortcode_, multiplier_):
    for line in fileinput.input(sim_config_dst_, inplace=1):
        line = line.rstrip()
        print(line)
        if shortcode_ in line:
            print(str("SIMCONFIG USE_SEQ2CONF_MULTIPLIER " + str(multiplier_)))

def send_email(mail_address, mail_body):
    msg = MIMEMultipart()
    msg["To"] = mail_address
    msg["From"] = mail_address
    msg["Subject"] = "New Config Created (Seq2conf Sensitivity Metrics)"
    msg.attach(MIMEText(mail_body, 'html'))


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




"""  command line arguments """
parser = argparse.ArgumentParser(description="checks the sensivity of strats wrt seq2conf delays")
parser.add_argument('-shc', help='shortcode', required=True, dest='shc_')
parser.add_argument('-sdate', help='start date', required=True, dest='start_date_')
parser.add_argument('-edate', help='end date', required=True, dest='end_date_')
parser.add_argument('-dtlist', help='date list', required=False, dest='date_list_')
parser.add_argument('-configdir', help='config directory', required=True, dest='config_dir_')
parser.add_argument('-sort', help='sorting based on volume and pnl', required=False, dest='sort_')
parser.add_argument('-results', help='stores the final dataframe in a file', required=False, dest='results_')
parser.add_argument('-send_email', help='Sends results as email to dvctrader', required=False, dest='send_email_')
args = parser.parse_args()

random_number_ = randint(0, 1000000000)
sim_config_dir = execs.execs().sim_config_base
shortcode_ = args.shc_
start_date_ = args.start_date_
end_date_ = args.end_date_
sort_ = args.sort_
config_dir_ = args.config_dir_
date_list_ = args.date_list_
results_ = args.results_
current_user_ = getpass.getuser()
send_email_ = args.send_email_


results_dir_ = os.path.join("/spare/","local" ,current_user_, "robustness_check_logs",str(random_number_))

""" setting environment variable """
os.environ["WORKDIR"] = "tmp"
os.environ["DEPS_INSTALL"] = os.path.join("/spare/local/",current_user_,"temp_sim_config",str(random_number_))

""" creating directory """
dir_ = os.path.join(os.environ["DEPS_INSTALL"], sim_config_dir)
makedirs(dir_, 0o777, exist_ok=True)
makedirs(results_dir_, 0o777, exist_ok=True, )

"""  file path """
sim_config_file_path = os.path.join(dir_, "sim_config.txt")

copy_sim_config(sim_config_dir, current_user_)
copy_market_model_info(current_user_)

base_path_market_model_info_file_ = "baseinfra/OfflineConfigs/MarketModelInfo/market_model_info.txt"
market_model_info_dst_ = os.path.join(os.environ["DEPS_INSTALL"], base_path_market_model_info_file_)
sim_config_dst_ = os.path.join(os.environ["DEPS_INSTALL"], sim_config_dir, "sim_config.txt")

multipliers_ = [0.5,1,10,50,100]

print("Logging at " +results_dir_+"/logs")

date_list_path = ""
"""storing date_list in file"""
if(date_list_):
    date_list_ = date_list_.split(",")
    date_list_path = os.path.join(results_dir_, "date_list")
    with open(date_list_path, mode='w', encoding='utf-8') as date_file:
        date_file.write('\n'.join(date_list_))
    date_file.close()


"""running simulations for each multiplier"""
for multiplier_ in multipliers_:
    copy_market_model_info(current_user_)
    copy_sim_config(sim_config_dir, current_user_)
    insert_multiplier_addend_in_sim_config(sim_config_dst_, shortcode_, multiplier_)
    f = open(results_dir_ + "/logs", "w+")

    current_result_directory_ = os.path.join(results_dir_, str(multiplier_))
    f.write("RUNNING FOR SEQ DELAY MULTIPLIER : " + str(multiplier_))

    makedirs(current_result_directory_, 0o777, exist_ok=True, )
    run_simulations_cmd_ = ""
    if(date_list_):
        run_simulations_cmd_ = [execs.execs().run_simulations, shortcode_, config_dir_, start_date_, end_date_,
                                current_result_directory_, "-dtlist", date_list_path, "-d", "0", "--nogrid"]
    else :
        run_simulations_cmd_ = [execs.execs().run_simulations, shortcode_, config_dir_, start_date_, end_date_,
                            current_result_directory_, "-d", "0", "--nogrid"]

    f.write(("RUN_SIMULATIONS_CMD " + ' '.join(run_simulations_cmd_)))
    out = subprocess.Popen(' '.join(run_simulations_cmd_), shell=True, stdout=subprocess.PIPE)
    run_simulations_output = out.communicate()[0].decode('utf-8').strip()

    f.write(("RUN_SIM_OUTPUT " + run_simulations_output))
    f.write("\n+++================================================================================+++\n")
    f.close()

"""running summarize strategies for each multiplier"""
for multiplier_ in multipliers_:
    current_result_directory_ = os.path.join(results_dir_, str(multiplier_))
    summarize_strategy_cmd_ = [execs.execs().summarize_strategy, shortcode_, config_dir_, current_result_directory_, start_date_, end_date_,"IF", "kCNAPnlSharpeAverage", "0", "IF", "1" ]
    out = subprocess.Popen(' '.join((summarize_strategy_cmd_)), shell=True, stdout=subprocess.PIPE)
    summarize_strategy_cmd_output = out.communicate()[0].decode('utf-8').strip()
    summary_file = os.path.join(results_dir_,"summary",str(multiplier_))
    makedirs(summary_file, 0o777, exist_ok=True)


    f = open(summary_file+"/summary","w+")
    f.write(summarize_strategy_cmd_output)
    f.close()

    summary_file_path = os.path.join(summary_file,"summary")
    temp = pd.read_csv(summary_file_path, delim_whitespace=True,header=None)
    temp = temp.set_index([1])
    temp_avg_pnl = temp.iloc[:,1]
    temp_avg_pnl = temp_avg_pnl.rename(str(multiplier_)+"_AP")
    temp_std_dev = temp.iloc[:,2]
    temp_std_dev = temp_std_dev.rename(str(multiplier_) + "_SDP")
    temp_avg_vol = temp.iloc[:, 3]
    temp_avg_vol = temp_avg_vol.rename(str(multiplier_) + "_AV")
    if(multiplier_ == 0.5):
        total_pnls = temp_avg_pnl
        total_std_dev = temp_std_dev
        total_avg_vol = temp_avg_vol
    else:
        total_pnls = pd.concat([total_pnls, temp_avg_pnl], axis=1)
        total_std_dev = pd.concat([total_std_dev, temp_std_dev], axis=1)
        total_avg_vol = pd.concat([total_avg_vol, temp_avg_vol], axis=1)

total_pnls['SD_AP'] = total_pnls.std(axis=1)
total_avg_vol['SD_AV'] = total_avg_vol.std(axis=1)

final_res = pd.concat([total_pnls, total_std_dev, total_avg_vol], axis=1)

pd.options.display.width = 1000
pd.options.display.max_colwidth = 60

final_res.sort_values(['SD_AP'], ascending=[False])
final_res.index.name = 'Config Name'
print(final_res)

if(send_email_):
    send_email("nseall@tworoads.co.in", final_res.to_html())

"""writing to the final data frame to a file"""
if(results_ != None):
    final_res.to_csv(results_, sep='\t', encoding='utf-8')

print("Deleting temp results and logs")
print("AP : Average PNL\nSDP : Standard Deviation in PNL\nAV : Average Volume\nSD : Standard Deviation")
""" removing the dir"""
shutil.rmtree(os.environ["DEPS_INSTALL"])
"""removing temp results"""
shutil.rmtree(results_dir_)

""" removing env variables """
del os.environ["WORKDIR"]
del os.environ["DEPS_INSTALL"]
