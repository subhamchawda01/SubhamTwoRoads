import argparse
import sys
import os

from create_strat_permutations import create_strat_permutations
from run_simulations import run_simulations

exec_to_run = "/home/dvctrader/stable_exec/tradeengine"
num_workers = 5
start_time = "IST_918"
end_time = "IST_1525"
live_file_name = "LIVE_FILE.csv"

parser = argparse.ArgumentParser()
parser.add_argument('strat_folder', help='Strat folder to run permutations on')
parser.add_argument('expression', help='Pattern for folders to consider from strat folder')
parser.add_argument('out_folder', help='Folder to store permutations')
parser.add_argument('permutation_file', help='Permutation file')
parser.add_argument('end_date', help='Date till which results are to be calculated')
parser.add_argument('num_days', help='Number of days on which results are calculated')
parser.add_argument('--start_time', help='Start Time of strategy')
parser.add_argument('--end_time', help='End time of strategy')
parser.add_argument('--exec_to_run', help='Full path of executable to run')
parser.add_argument('--num_workers', help='Number of parallel runs allowed')
parser.add_argument('--live_file_name', help='Name of live on which to run strategy')

args = parser.parse_args()

if args.strat_folder:
    strat_folder = args.strat_folder
else:
    sys.exit('Please provide input strat folder')

if args.expression:
    expression = args.expression
else:
    sys.exit('Please provide pattern to choose folder from')

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

if args.start_time:
    start_time = args.start_time

if args.end_time:
    end_time = args.end_time

if args.exec_to_run:
    exec_to_run = args.exec_to_run

if args.num_workers:
    num_workers = args.num_workers

if args.live_file_name:
    live_file_name = args.live_file_name

create_strat_permutations(strat_folder, expression, out_folder, permutation_file)

results_folder = os.path.join(out_folder, "results")
strat_list_file = os.path.join(out_folder, "strat_list")
strat_list_ = [os.path.join(out_folder,folder, live_file_name) for folder in os.listdir(out_folder)]

with open(strat_list_file, "w") as output:
    output.write('\n'.join(strat_list_))

run_simulations(strat_list_file, end_date, num_days, results_folder, start_time,
                end_time, exec_to_run, num_workers)
