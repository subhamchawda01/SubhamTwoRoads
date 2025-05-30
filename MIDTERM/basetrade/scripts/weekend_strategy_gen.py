import os
import sys
import random
import subprocess
import shlex

FILE_PATH='/home/dvctrader/modelling/stratwork/strategy_generation_configs'


def run_config():
    with open(FILE_PATH, 'r') as config_file:
        os.environ['GRID_USERNAME']="animesh"
        os.environ['GRID_PASSWORD']="animesh@pass435"
        commands_to_run = config_file.readlines()
        processes = set()
        for command in commands_to_run:
            processes.add(subprocess.Popen(shlex.split(command), stdout=sys.stdout, stderr=sys.stdout))

        for proc in processes:
            proc.wait()
            print("Successful")

run_config()