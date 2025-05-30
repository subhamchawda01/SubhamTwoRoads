#!/usr/bin/python

from datetime import date
import sys
import os
import subprocess
import math
import operator
import heapq
import argparse
import getpass
import time
import datetime
import numpy as np


sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.definitions import execs
from walkforward.print_strat_from_multiple_config_multiple_date import print_strat_from_multiple_config_multiple_date
from walkforward.wf_db_utils.db_handles import set_backtest

def get_list_of_outer_strategy_files(out_dir):
    fileList =[]
    for root, dirs, files in os.walk(out_dir, topdown=False):
        for name in files:
            fileList.append(os.path.join(root, name))
    return fileList

def get_inner_strat_file(File):
    outerFile = open(File, "r")
    outerFileArray = outerFile.read().strip().split()
    innerFileName = outerFileArray[1]
    parentDir =  os.path.dirname(File)
    date = parentDir.split("/")[-1]
    return [innerFileName,parentDir]



def generate_individual_shc_strats(fileList):
    innerFileList= [ get_inner_strat_file(file)  for file in fileList]


    for innerFile in innerFileList:
        fileSingleshc = open(innerFile[0],"r")

        strat_list=fileSingleshc.read().strip().split("\n")
        main_strat_line=strat_list[0]
        ParentDir = innerFile[1]
        globalExecLogic = main_strat_line.split(" ")[2]
        individualExecLogic = globalExecLogic.split("-")[2]
        single_shc_lines=strat_list[1:]
        for strat in single_shc_lines:
            stratArray = strat.split(" ")
            shortcode = stratArray[1]
            newStratArray = []
            newStratArray.extend(stratArray[0:2])
            newStratArray.append(individualExecLogic)
            newStratArray.extend(stratArray[2:len(stratArray)])
            newStrat = ' '.join(newStratArray)
            newStratName = ParentDir+"/single_shc_name_"+shortcode
            #print("NewStratName ",newStratName)
            newfile =open(newStratName,"w")
            newfile.write(newStrat)
            newfile.close()

def remove_structured_strats_from_folder(work_dir):
    for root, dirs, files in os.walk(work_dir, topdown=False):
        for name in files:
            if name.find('single_shc_name_') == -1:
                print("Removing :",name)
                os.remove(os.path.join(root, name))

def get_immediate_subdirectories(a_dir):
    return [name for name in os.listdir(a_dir)
            if os.path.isdir(os.path.join(a_dir, name))]



def generate_results(work_dir,start_date,end_date,result_dir):

    run_simulations_cmd_=[execs.execs().run_simulations,"ALL", work_dir, str(start_date), str(end_date),
    result_dir, "--nogrid"]
    out = subprocess.Popen(' '.join(run_simulations_cmd_), shell=True, stdout=subprocess.PIPE)
    run_simulations_output = out.communicate()[0].decode('utf-8').strip()




def fetch_individual_from_structured_strat(args):
    out_dir = print_strat_from_multiple_config_multiple_date(
        args.file_with_configs, args.start_date, args.end_date, args.use_days_file, args.skip_days_file, args.work_dir+"print_strat_dir/",
        args.days_to_look)

    filelist = get_list_of_outer_strategy_files(out_dir)

    generate_individual_shc_strats(filelist)

    remove_structured_strats_from_folder(out_dir)
    subdirList = get_immediate_subdirectories(out_dir)
    for name in subdirList:
        generate_results(os.path.join(out_dir, name), name, name, args.work_dir+"results_dir/")





if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument('-cfile', dest='file_with_configs', help="File with list of configs", type=str, required=True)
    parser.add_argument('-sd', dest='start_date', help="Start Date", type=int, required=True)
    parser.add_argument('-ed', dest='end_date', help='End Date', type=int, required=True)
    parser.add_argument('-use', dest='use_days_file', help='Use Days file', type=str, required=False)
    parser.add_argument('-skip', dest='skip_days_file', help='Skip Days file', type=str, required=False)
    parser.add_argument('-work_dir', dest='work_dir', help='Working Directory Location', type=str, required=True)
    parser.add_argument('-look_days', dest='days_to_look',
                        help='Number of past models to look for', type=int, required=False)
    parser.add_argument('-b', dest='use_backtest', help='Use backtest results', type=int, required=False)


    args = parser.parse_args()


    if args.use_backtest == 1 or 'USE_BACKTEST' in os.environ:
        set_backtest(True)
        os.environ['USE_BACKTEST'] = "1"

    fetch_individual_from_structured_strat(args)




