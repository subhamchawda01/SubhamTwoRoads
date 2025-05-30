import argparse
import sys
import pandas as pd
import os
from run_simulations import run_simulations

import random
import time

def check_exec_stability(strat_list_file, date_file, num_days,result_folder_id, base_folder, 
                     start_time, end_time, exec_to_run, num_workers):
    
    random.seed(time.time())
    results_folder = '/home/dvctrader/RESULTS_FRAMEWORK/sim_results_' +result_folder_id
    run_simulations(strat_list_file, date_file, num_days, results_folder, start_time, end_time, exec_to_run, num_workers)
    strat_names = "/spare/local/logs/tradelogs/strat_list_" + str(random.randint(1000000, 9000000))
    strategy_ =  open(strat_names,'w')
    live_file_ = open(strat_list_file,'r')

    for line in live_file_:
        strat_ = line.split(os.sep)[-2]
        # print(strat_)
        strategy_.write(strat_ + "\n")

    strategy_.close()
    
    cmd = "python /home/dvctrader/stable_exec/scripts/analyze_results.py {0} {1} {2}".format(base_folder,results_folder, strat_names)
    os.system(cmd)
    os.remove(strat_names)




if __name__ == "__main__":

    strat_list_file = "/home/dvctrader/RESULTS_FRAMEWORK/strats/live_file_list"
    base_folder = "/home/dvctrader/RESULTS_FRAMEWORK/base_results"
    date_file = "/home/dvctrader/RESULTS_FRAMEWORK/strats/dates_list"
    num_workers = 5
    start_time = "IST_918"
    end_time = "IST_1525"
    num_days="0"

    parser = argparse.ArgumentParser()
    parser.add_argument('exec_to_run', help='Full path of executable to run')
    parser.add_argument('result_folder_id', help='Result folder id')
    parser.add_argument('--strat_list_file', help='File containing list of live files')
    parser.add_argument('--date_file', help='Date till which results are to be calculated or file containing dates')
    parser.add_argument('--base_folder', help='Folder where base is located')
    
    parser.add_argument('--start_time', help='Start Time of strategy')
    parser.add_argument('--end_time', help='End time of strategy')
    parser.add_argument('--num_workers', help='Number of parallel runs allowed')

    args = parser.parse_args()

    if args.exec_to_run:
        exec_to_run = args.exec_to_run
    else:
        sys.exit('Please provide executable to run')

    if args.result_folder_id:
        result_folder_id = args.result_folder_id
    else:
        sys.exit('Please provide result folder id')


    if args.strat_list_file:
        strat_list_file = args.strat_list_file
    
    if args.date_file:
        date_file = args.date_file
    
    if args.base_folder:
        base_folder = args.base_folder
   
    if args.start_time:
        start_time = args.start_time

    if args.end_time:
        end_time = args.end_time

    if args.num_workers:
        num_workers = args.num_workers

    check_exec_stability(strat_list_file, date_file, num_days,result_folder_id, base_folder, start_time,
                    end_time, exec_to_run, num_workers)
