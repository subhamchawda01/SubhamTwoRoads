#!/usr/bin/env python

"""

Script to find the best params for given config.
Currently optimization is pnl space

"""

import os
import sys
import json
import random
import argparse
import subprocess

sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.definitions.config import config
from walkforward.definitions.execs import paths
from walkforward.definitions.execs import execs

from walkforward.utils.date_utils import calc_prev_week_day
from walkforward.utils.permute_config import permute_config
from walkforward.utils.process_config_utils import prune_config

from walkforward.wf_db_utils.db_handles import set_backtest
from walkforward.wf_db_utils.dump_config_to_db import dump_config_to_db

from walkforward.compute_strat_for_config_and_date import compute_strat_for_config_and_date
from walkforward.utils.get_modelparams_for_config_util import get_modelparams_for_config

parser = argparse.ArgumentParser()
parser.add_argument('-c', dest='configname', help="the walk-forward-config path", type=str, required=True)
parser.add_argument('-ed', dest='end_date', help="training end-date", type=int, required=True)
parser.add_argument('-l', dest='lookback_days', help='lookback-days', type=int, required=True)
parser.add_argument('-d', dest='distributed', help='weather to distributed run-simulations', type=int,
                    default=0, required=True)

args = parser.parse_args()

# set_backtest(True)

"""
Create local working directories

"""

work_dir = os.path.join(paths().shared_ephemeral_fbpa, str(random.randint(10, 1000000)))

configs_dir = os.path.join(work_dir, 'configs')
local_results_base_dir = os.path.join(work_dir, 'local_results_base_dir')
logfilename = os.path.join(work_dir, 'main_log_file.txt')

if not os.path.exists(work_dir):
    os.makedirs(work_dir)
    os.makedirs(configs_dir)
    os.makedirs(local_results_base_dir)
else:
    print("Rare. The working directory already exists")

print(('LogFile: ' + logfilename, ))

logfile = open(logfilename, 'w')

# will need in run_simulations
start_date = calc_prev_week_day(args.end_date, args.lookback_days - 1)
shortcode = ""

config_json = open(args.configname).read()
#print ("OrignalConfig: ", config_json)


cfg = config.initialize()
cfg.config_json = config_json

# update the fields of the config from it's json value
cfg.update_config_from_its_json()

shortcode = cfg.shortcode

# get the list of configs based on the fields here
config_name_list = permute_config(cfg, args.configname, configs_dir)


"""

insert all the configs into test database,
Keep list of files as record to delete them later on

"""

base_configname = os.path.basename(args.configname)
dump_config_to_db(base_configname, "INVALID", cfg, False)

for local_config in config_name_list:
    local_config_struct = config.initialize()
    # read the config json from file and
    # update the config fields
    with open(local_config) as local_config_file:
        local_config_struct.config_json = local_config_file.read()
        # print ("JSON: ", local_config_struct.config_json)
        local_config_struct.update_config_from_its_json()
        base_local_config = os.path.basename(local_config)
        dump_config_to_db(base_local_config, 'INVALID', local_config_struct, False)


"""

Compute the strats for each configs
Currently recommended to not use for type 4 configs as it can potentially take a lot of time

"""

for local_config in config_name_list:
    compute_strat_for_config_and_date(local_config, args.end_date, args.lookback_days, False)
    logfile.write('calling compute_strat_for_config_and_date' + str(args.end_date) + ' ' + str(args.lookback_days))
    date_to_modelparam_pair = get_modelparams_for_config(local_config, args.end_date, args.lookback_days)
    for date in list(date_to_modelparam_pair.keys()):
        model_param_pair = date_to_modelparam_pair[date]
        logfile.write('CONFIG: %s %d %s %s\n' % (local_config, date, model_param_pair[0], model_param_pair[1]))


"""

Run simulations on the configs

"""


os.environ['USE_BACKTEST'] = '1'

run_simulations_cmd = [execs().run_simulations, shortcode, configs_dir, str(start_date), str(args.end_date),
                       local_results_base_dir, '-d', str(int(args.distributed))]

logfile.write(' '.join(run_simulations_cmd))

out = subprocess.Popen(' '.join(run_simulations_cmd), shell=True, stdout=subprocess.PIPE)
run_simulations_out = out.communicate()[0].decode('utf-8').strip()
#run_simulations_out= subprocess.check_output(run_simulations_cmd)
print(run_simulations_out)

logfile.close()

"""

Clear directories (if needed)

"""

print("Pruning from the database..")
prune_config(args.configname, False)
for local_config in config_name_list:
    prune_config(local_config, False)
