#!/usr/bin/env python

import os
import json
import sys


def split_json_file(strat_file, num_strat_per_file):
    """
        The function splits a JSON file into several JSON files (containing less 
        than or equal to 1200 strategies needed for grid api to work

        Keyword arguments:
            strat_file : JSON fileThe JSON file containing all the strategies for a particular job.
            num_strat_per_file :The int defining the threshold on the number of strategies per JSON file.

    """

    with open(strat_file, 'r') as infile:
        json_dict = json.load(infile)
    assert isinstance(json_dict, dict), 'Invalid file configuration'

    try:
        json_dict["job"]
    except:
        print("Could not read job")

    try:
        json_dict["strategies"]
    except:
        print("Could not read strategies")

    worker_job = json_dict["job"]

    strat_list = json_dict["strategies"]

    assert isinstance(strat_list, list), 'Invalid strat_ist configuration'

    curr_num_strat = 0
    strat_dict = {}
    file_no = 0
    basename = os.path.basename(strat_file)
    file_prefix = os.path.splitext(basename)[0]
    strat_dict["job"] = worker_job
    strat_dict["strategies"] = []

    for strat in strat_list:
        if curr_num_strat >= num_strat_per_file:
            temp_json_file = os.path.join(os.path.dirname(strat_file), file_prefix) + "_" + str(file_no) + ".json"
            with open(temp_json_file, 'w') as outfile:
                json.dump(strat_dict, outfile, indent=4, separators=(',', ': '))
                print(temp_json_file)
            curr_num_strat = 0
            strat_dict = {}
            strat_dict["job"] = worker_job
            strat_dict["strategies"] = []
            file_no = file_no + 1

        assert isinstance(strat, dict), 'Invalid strat configuration'
        strat_dict["strategies"].append(strat)
        curr_num_strat = curr_num_strat + 1

    if len(strat_dict["strategies"]) > 0:
        temp_json_file = os.path.join(os.path.dirname(strat_file), file_prefix) + "_" + str(file_no) + ".json"
        with open(temp_json_file, 'w') as outfile:
            json.dump(strat_dict, outfile, indent=4, separators=(',', ': '))
            print(temp_json_file)


if __name__ == '__main__':
    split_json_file(sys.argv[1], 100)
