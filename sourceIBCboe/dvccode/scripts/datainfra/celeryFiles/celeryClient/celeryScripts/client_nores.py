#!/usr/bin/python
'''
Input:
Timeout Full-Path-to-Executable Arg1 Arg2 ...

Sample Execution:
python client.py timeout /pathtolog/log.txt queue_name program_type /fullpath/a.out arg1 arg2 arg3
Example:
python client_nores.py autoscalegroupmanual ls -l

This function calls exec_func which then executes the task in worker machine.
This file can be further modified to include routing logic
'''

import sys
import os
from proj.workerProg import exec_func
from proj.workerProg import exec_func_nores
from celery.exceptions import WorkerLostError
from celery.exceptions import TimeoutError
import traceback
import sys
import subprocess
from support import exec_function
import datetime
import socket

if len(sys.argv) < 3:
   print("Error: Too Less Arguments")
   print("Usage: python " + sys.argv[0] + " file_name queue_name file/command_to_run")
   quit()

#get the name of log file
exception_path = '/mnt/sdf/logs/exceptions.txt'
log_cmd = '/mnt/sdf/logs/cmd.txt'

filename = sys.argv[1]
queue_name = sys.argv[2]
is_file_str = sys.argv[3]
is_file = is_file_str.startswith("file:")
program = sys.argv[3:]

if not is_file:
    result = exec_func_nores.apply_async(args=[" ".join(program), filename], queue=queue_name, routing_key=queue_name, retry=True, retry_policy={'max_retries' : 3})
    print("Task: ", " ".join(program))
    print("Id: ", result.id)
else:
    prog_file = sys.argv[3][5:]
    with open(prog_file) as f:
        lines = f.readlines()
        for line in lines:
            program = line.split()
            result = exec_func_nores.apply_async(args=[" ".join(program), filename], queue=queue_name, routing_key=queue_name, retry=True, retry_policy={'max_retries' : 3})
            print("Task: ", " ".join(program))
            print("Id: ", result.id)
