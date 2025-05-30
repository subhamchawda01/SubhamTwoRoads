#!/usr/bin/env python
import sys
import os
from proj.workerProg import local_exec_func

if len(sys.argv) < 2:
   print("Error: Too Less Arguments")
   print("Usage: python " + sys.argv[0] + " run_my_job.py args")
   quit()

program = " ".join(sys.argv[1:])
queue_name = "localjobs"
time_out = 15*60 #3 minutes
soft_time_limit_num = 15*60
time_limit_num = 16*60

def execute(program):
    result = local_exec_func.apply_async(args=[program], queue=queue_name, routing_key=queue_name, retry=True, retry_policy={'max_retries' : 3}, soft_time_limit = soft_time_limit_num, time_limit = time_limit_num)
    [output, err, ret_code] = result.get(timeout = time_out)
    if err != None or ret_code != 0:
         print("=== ERROR OCCURED ==")
    print("Output: \n", output)
    if err != None:
        print("Err: \n", err)
        print("Return Code: ", ret_code)

execute(program)
