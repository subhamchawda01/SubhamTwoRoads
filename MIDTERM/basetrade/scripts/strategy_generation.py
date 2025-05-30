import os
import random
import subprocess
import shlex
import sys
from grid.client.resources import JobResource
from grid.client.api import GridClient

LIMIT=2
FILE_PATH='/home/dvctrader/modelling/stratwork/strategy_generation_configs'
LOG_FILE='/home/dvctrader/ankitp/strategy_generation.log'

def run_config():
    with open(FILE_PATH, 'r') as config_file:
        os.environ['GRID_USERNAME']="animesh"
        os.environ['GRID_PASSWORD']="animesh@pass435"
        commands_to_run = config_file.readlines()
        processes = set()
        num_of_commands = len(commands_to_run)
        config_ran=[]
        i=0
        while i<LIMIT:
            config = random.randint(0,num_of_commands-1)
            if config not in config_ran:
                config_ran.append(config)
                processes.add(subprocess.Popen(shlex.split(commands_to_run[config]), stdout=sys.stdout, stderr=sys.stdout))
                i=i+1

        for proc in processes:
            proc.wait()
            print("Successful")



client=GridClient("http://10.1.4.15:5000", "animesh", "animesh@pass435")
job_resource=JobResource()
num_grid_tasks=job_resource.queued_tasks()
num_grid_tasks=num_grid_tasks['tasks']

if num_grid_tasks == 0:
    run_config()
else:
    print("Grid Busy")



