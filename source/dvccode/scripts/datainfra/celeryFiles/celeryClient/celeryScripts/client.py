#!/usr/bin/env python
'''
Input:
Timeout Full-Path-to-Executable Arg1 Arg2 ...

Sample Execution:
python client.py timeout /pathtolog/log.txt queue_name /fullpath/a.out arg1 arg2 arg3
Example:
python client.py 2 /tmp/log.txt autoscalegroup list_files ls -l

This function calls exec_func which then executes the task in worker machine.
This file can be further modified to include routing logic
'''

import sys
import os
from proj.workerProg import exec_func
from celery.exceptions import WorkerLostError
from celery.exceptions import TimeoutError
import traceback
import sys
import subprocess
from support import exec_function
import datetime
import socket

if len(sys.argv) < 4:
   print("Error: Too Less Arguments")
   print("Usage: python " + sys.argv[0] + " timeout /pathtolog/log.txt queue_name hashing_key command_to_run")
   quit()

#get the name of log file
filename = sys.argv[2]
directory = os.path.dirname(filename)
exception_path = '/mnt/sdf/logs/exceptions.txt'
log_cmd = '/mnt/sdf/logs/cmd.txt'

#create directory in which log file lies
if not (os.path.exists(directory) or directory == ''):
    os.makedirs(directory)
logfile = open(filename, 'w+')

time_out = int(sys.argv[1])
queue_name = sys.argv[3]
hashing_key = sys.argv[4]
is_file_str = sys.argv[5]
is_file = is_file_str.startswith("file:")

def write_exception( err, start, i, queue_name, traceback_info ):
   now = datetime.datetime.now()
   f = open(exception_path,'a')
   f.write('Curr time: ' + str(now) + '\n')
   f.write('Started at: ' + str(start) + '\n')
   f.write(str(os.environ.get('DVC_JOB_ID')) + '\n')
   f.write(socket.gethostname() + '\n')
   f.write(traceback_info + '\n')
   f.write("\"Exception: " + str(err) + " type: " + type(err).__name__ + ", retry no:" + str(i) +     ", queue: " + queue_name + ", program: " + str(program) + "\"\n\n")
   f.close()

ret_code = 0
def execute(program):
 start = datetime.datetime.now()
 err = Exception()
 for i in range(0, 3):
 #   print i
   try:
      f = open(log_cmd,'a')
      f.write(str(os.environ.get('DVC_JOB_ID')) + '\n')
      f.write(str(program) + '\n\n')
      f.close()

      #send task to celery
      result = exec_func.apply_async(args=[program], queue=queue_name, routing_key=queue_name, retry=True, retry_policy={'max_retries' : 3})
      print("Id: ", result.id)

      #get result within given time out
      [output, err, ret_code] = result.get(timeout = time_out)
      
      #write output and error(if exists) in logfile
      logfile.write(str(output))
      if err != None:
         logfile.write(str(err))

      return [output, err, ret_code]
   except WorkerLostError as err:# WorkerLostError:
      print('Workerlost Exception')
      print("Program: ", " ".join(program))
      #Write Exception to File
      write_exception( err, start, i, queue_name, str(traceback.format_exc()) )
      pass
   except Exception as err:
      print("Program: ", " ".join(program))
      print(err)  
      write_exception( err, start, i, queue_name, str(traceback.format_exc()) ) 
 #throw error in case there is exception
 raise err

err = Exception()
if not is_file:
    program = sys.argv[5:]
    try:
        [output, err, ret_code] = execute(program)
        sys.exit(ret_code)
    except Exception as err:
        raise err
else:
    prog_file = sys.argv[5][5:]
    with open(prog_file) as f:
        lines = f.readlines()
        for line in lines:
            try:
                print("Program: ", line.rstrip())
                program = line.split()
                [output, err, ret_code] = execute(program)
                if ret_code != 0:
                    ret_code_final = ret_code
            except Exception as err:
                pass
logfile.close()
