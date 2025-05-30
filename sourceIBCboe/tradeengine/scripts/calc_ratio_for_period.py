import argparse
import sys
import pandas as pd
import os
import concurrent.futures
import subprocess
import random
from utils import get_list_of_dates_



def execute_command(script_to_run,date,pid,start_time,end_time,tag,live_file_path):

    print(' '.join([script_to_run,date,pid,start_time,end_time,tag,live_file_path]))
    subprocess.call(['sh',script_to_run,date,pid,start_time,end_time,tag,live_file_path])

if __name__ == "__main__":

    script_to_run = "/home/dvctrader/usarraf/tradeengine/scripts/calc_ratio.sh"
    num_workers = 2
    start_time = "IST_915"
    end_time = "IST_930"
    tag="StartRatio"
    live_file_path="/home/dvctrader/CONFIG_SET/CONFIG_FUT1_RATIO_CALCULATOR/LIVE_FILE.csv"

    parser = argparse.ArgumentParser()
    parser.add_argument('end_date', help='Date till which results are to be calculated')
    parser.add_argument('num_days', help='Number of days on which results are calculated')
    parser.add_argument('--tag', help='Folder tag where result will be restored')
    parser.add_argument('--start_time', help='Start Time of strategy')
    parser.add_argument('--end_time', help='End time of strategy')
    parser.add_argument('--live_file_path', help='Path of live file to run')
    parser.add_argument('--script_to_run', help='Full path of executable to run')
    parser.add_argument('--num_workers', help='Number of parallel runs allowed')

    args = parser.parse_args()

    if args.end_date:
        end_date = args.end_date
    else:
        sys.exit('Please provide date till which results are to be calculated')

    if args.num_days:
        num_days = int(args.num_days)
    else:
        sys.exit('Please provide number of days on which results are calculated')

    if args.tag:
        tag = args.tag

    if args.start_time:
        start_time = args.start_time

    if args.end_time:
        end_time = args.end_time

    if args.script_to_run:
        script_to_run = args.script_to_run

    if args.num_workers:
        num_workers = args.num_workers

    if args.live_file_path:
        live_file_path = args.live_file_path

    runtime_id = random.randint(1000000, 9000000)

    dates_list = get_list_of_dates_(end_date, num_days)

    with concurrent.futures.ThreadPoolExecutor(max_workers=int(num_workers)) as executor:
        futures = [executor.submit(execute_command, script_to_run ,x, str(runtime_id), start_time, end_time, tag, live_file_path) for x in dates_list]
        for future in concurrent.futures.as_completed(futures):
            try:
                print(future.result())
            except Exception as e:
                print(e)
