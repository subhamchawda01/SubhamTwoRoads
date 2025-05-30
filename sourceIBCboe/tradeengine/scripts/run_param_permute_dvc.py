import argparse
import sys
import os

from create_strat_permutations_dvc import create_strat_permutations
from run_simulations_dvc import run_simulations

exec_to_run = "/home/dvctrader/stable_exec/tradeengine"
num_workers = 5
start_time = "IST_918"
end_time = "IST_1525"
live_file_name = "LIVE_FILE.csv"

parser = argparse.ArgumentParser()
parser.add_argument('strat_file', help='Strat folder to run permutations on')
parser.add_argument('out_folder', help='Folder to store permutations')
parser.add_argument('permutation_file', help='Permutation file')
parser.add_argument('end_date', help='Date till which results are to be calculated')
parser.add_argument('num_days', help='Number of days on which results are calculated')
parser.add_argument('--exec_to_run', help='Full path of executable to run')
parser.add_argument('--num_workers', help='Number of parallel runs allowed')

args = parser.parse_args()

if args.strat_file:
    strat_file = args.strat_file
else:
    sys.exit('Please provide input strat file')

if args.out_folder:
    out_folder = args.out_folder
else:
    sys.exit('Please provide folder to store permutations')

if args.permutation_file:
    permutation_file = args.permutation_file
else:
    sys.exit('Please provide permutation file')

if args.end_date:
    end_date = args.end_date
else:
    sys.exit('Please provide date till which results are to be calculated')

if args.num_days:
    num_days = args.num_days
else:
    sys.exit('Please provide number of days on which results are calculated')

if args.exec_to_run:
    exec_to_run = args.exec_to_run

if args.num_workers:
    num_workers = args.num_workers

create_strat_permutations(strat_file, out_folder, permutation_file)

out_folder = os.path.abspath(out_folder)

results_folder = os.path.join(out_folder, "results")
strat_list_file = os.path.join(out_folder, "strat_list")
strat_folder = os.path.join(out_folder, "strats")
strat_list_ = [os.path.join(strat_folder, f) for f in os.listdir(strat_folder)]

with open(strat_list_file, "w") as output:
    output.write('\n'.join(strat_list_))

run_simulations(strat_list_file, end_date, num_days, results_folder, exec_to_run, num_workers)
