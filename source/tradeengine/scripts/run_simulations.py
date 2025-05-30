import argparse
import sys
import pandas as pd
import os
import concurrent.futures
from utils import get_list_of_dates_
from store_single_run import get_results_for_single_run,add_result_line


def get_strat_name_from_live_file(live_file):
    strat_name = os.path.basename(os.path.dirname(live_file))
    if strat_name == "" :
        return "DEFAULT_NAME"
    else:
        return strat_name

def run_simulations(strat_list_file_, end_date_or_file_, num_days_, results_folder_, start_time_,
                    end_time_, exec_to_run_, num_workers_, pid=1234,bar_data=0,exchange="NSE"):

    # print("hi")
    dates_list_ = []
    if not os.path.exists(end_date_or_file_):
        dates_list_ = get_list_of_dates_(end_date_or_file_, num_days_)
    else:
        for line in open(end_date_or_file_,"r"):
            dates_list_.extend([line.strip()])

    with open(strat_list_file_) as f:
        live_file_list_ = f.readlines()
        # you may also want to remove whitespace characters like `\n` at the end of each line
    live_file_list_ = [y.strip() for y in live_file_list_]
    strat_name_list_ = [get_strat_name_from_live_file(y) for y in live_file_list_]

    arg_list_ = [(x, y[0], y[1]) for x in dates_list_ for y in list(zip(live_file_list_, strat_name_list_))]

    if not os.path.exists(results_folder_):
        os.makedirs(results_folder_)
    with concurrent.futures.ThreadPoolExecutor(max_workers=int(num_workers_)) as executor:
        futures = [executor.submit(get_results_for_single_run, exec_to_run_, t[1], t[0],
                                   start_time_, end_time_, t[2], results_folder_,pid,bar_data,exchange) for t in arg_list_]
        for future in concurrent.futures.as_completed(futures):
            try:
                for result in future.result():
                    if len(result) > 4:
                        print(result[0],result[1],result[2],result[3],result[4])
                        add_result_line(result[0],result[1],result[2],result[3],result[4])
            except Exception as e:
                print(e)



if __name__ == "__main__":

    exec_to_run = "/home/dvctrader/stable_exec/tradeengine"
    num_workers = 5
    start_time = "IST_918"
    end_time = "IST_1525"
    pid = 1234
    bar_data = 0
    exchange = "NSE"

    parser = argparse.ArgumentParser()
    parser.add_argument('strat_list_file', help='File containing list of live files')
    parser.add_argument('end_date', help='Date till which results are to be calculated')
    parser.add_argument('num_days', help='Number of days on which results are calculated')
    parser.add_argument('results_folder', help='Folder to store results')
    parser.add_argument('--start_time', help='Start Time of strategy')
    parser.add_argument('--end_time', help='End time of strategy')
    parser.add_argument('--exec_to_run', help='Full path of executable to run')
    parser.add_argument('--num_workers', help='Number of parallel runs allowed')
    parser.add_argument('--pid', help='Constant pid for the process')
    parser.add_argument('--bar_data', help='Bar Data mode for the process')
    parser.add_argument('--exchange', help='Exchange for the process')

    args = parser.parse_args()

    if args.strat_list_file:
        strat_list_file = args.strat_list_file
    else:
        sys.exit('Please provide file containing list of live files')

    if args.end_date:
        end_date = args.end_date
    else:
        sys.exit('Please provide date till which results are to be calculated')

    if args.num_days:
        num_days = args.num_days
    else:
        sys.exit('Please provide number of days on which results are calculated')

    if args.results_folder:
        results_folder = args.results_folder
    else:
        sys.exit('Please provide folder to store results')

    if args.start_time:
        start_time = args.start_time

    if args.end_time:
        end_time = args.end_time

    if args.exec_to_run:
        exec_to_run = args.exec_to_run

    if args.num_workers:
        num_workers = args.num_workers

    if args.pid:
        pid = args.pid    

    if args.bar_data:
        bar_data = args.bar_data

    if args.exchange:
        exchange = args.exchang
        
    run_simulations(strat_list_file, end_date, num_days, results_folder, start_time,
                    end_time, exec_to_run, num_workers, pid,bar_data,exchange)
