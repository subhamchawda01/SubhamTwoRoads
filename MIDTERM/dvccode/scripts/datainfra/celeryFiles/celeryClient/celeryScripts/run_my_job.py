#!/usr/bin/env python
'''
Input:
Command to Execute

Sample Execution:
python run_my_job.py command_to_execute
Example:
python run_my_job.py ls -l

This function calls exec_job which then executes the task in worker machine.
This file can be further modified to include routing logic
'''

import sys
import os
from proj.workerProg import exec_job, exec_job_nores, exec_branch, group_clean_and_notify
from celery.exceptions import WorkerLostError
from celery.exceptions import TimeoutError
import traceback
import sys
import subprocess
from support import exec_function
import datetime
import socket
import argparse
import getpass
from celery import group, chord
import string
from support import get_curr_tasks

parser = argparse.ArgumentParser()
parser.add_argument("--notify", "-n", help="Notifies required user. Mandatory argument")
parser.add_argument("--cmd", "-c", help="Command to execute")
parser.add_argument("--cmdfile", "-f", help="File containing commands to execute")
parser.add_argument("--wait", "-w", help="Do/Donot (0/1) wait for output")
parser.add_argument("--nomail", "-m", help="Do/Donot (0/1) mail after completion")
parser.add_argument("--time_limit", "-t", help="Maximum time in seconds for which a program runs. Max 5 days")
parser.add_argument("--execs", "-e", help="Space separated list of custom execs")
parser.add_argument("--summary", "-s", help="Print only summary of tasks submitted. Used when cmdfile option used.")
parser.add_argument("--duration", "-d", help="Store time duration for tag")
parser.add_argument("--basetrade_branch", "-bb", help="Branch name for basetrade repo", default = "basetrade")
parser.add_argument("--dvccode_branch", "-cb", help="Branch name for dvccode module", default = "master")
parser.add_argument("--dvctrade_branch", "-tb", help="Branch name for dvccode module", default = "master")
parser.add_argument("--queue_name", "-q", help="Queue Name", default="autoscalegroupmanual")
parser.add_argument("--instant", "-i", help = "Testing purpose. Dont use", default=False)

#parser.add_argument("--samelog", "-s", help="Same log file for all commands given in a file.")
args = parser.parse_args()

user = getpass.getuser()
if len(sys.argv) < 2 or ( args.notify == None and ( user == 'dvctrader' or user == 'dvcinfra' ) ):
   parser.print_help()
   sys.exit(0)

notify = args.notify
if notify == None:
    notify = user

time_out = 7200
soft_time_limit_num = 5*24*60*60
if args.time_limit != None:
    soft_time_limit_num = int(args.time_limit)
time_limit_num = soft_time_limit_num + 2*60 #Wait for at max 2 minutes

##queue_name = "test"
queue_name = args.queue_name
prog = args.cmd
prog_file = args.cmdfile
is_file = (args.cmdfile != None)
nowait = (args.wait == None) or (args.wait == '1')
nomail = (args.nomail != None) and (args.nomail == '1')
execs = args.execs
custom_execs = (args.execs != None)
summary = (args.summary != None)
duration = args.duration
#samelog = (args.wait != None) and (args.wait == '1')
samelog = 0
start = datetime.datetime.now()
instant = args.instant
#Around twice the queue size
if queue_name == 'slow':
    MAX_MSGS = 250
    user_cmds_folder = '/media/shared/ephemeral16/slowCeleryCmnds'
elif queue_name == 'autoscalegroupmanual':
    MAX_MSGS = 60000
    user_cmds_folder = '/media/shared/ephemeral16/celeryCmnds'
else:
    MAX_MSGS = 15000
    user_cmds_folder = '/media/shared/ephemeral16/celeryCmnds'

# sim_strategy_
if duration != None and duration.startswith('sim_strategy'):
    #Change Time Limits, Need to add alerts for it
    soft_time_limit_num = 15*60 #15 mins
    time_limit_num = soft_time_limit_num + 2*60 #Wait for at max 2 minutes

    shc = duration[13:]
    [output, err, ret] = exec_function('/home/dvctrader/basetrade_install/bin/get_contract_specs ' + shc + ' 20161113 EXCHANGE')
    output.strip().decode('utf-8')
    exchange = output.split()[1]
    if exchange == 'ICE':
        print("Skipping products of ICE exchange for now. Product: ", shc)
        exit(0)

def subtitute_exec(final_execs, prog):
    prog_list = prog.split()
    final_prog = []
    for word in prog_list:
        added = False
        for myexec in final_execs:
            if word == os.path.basename(myexec):
                final_prog.append(myexec)
                added = True
                break
        if not added:
            final_prog.append(word)
    return " ".join(final_prog)

def get_exec_dir():
    [ exec_dir, err, ret ] = exec_function("date +%N")
    exec_dir = "/media/shared/ephemeral16/celerylogs/execs/" + exec_dir.strip().decode('utf-8')
    exec_function("mkdir -p " + exec_dir)
    exec_function("chmod a+wx " + exec_dir)
    return exec_dir

def get_scheduled_file_path():
    [ exec_file, err, ret ] = exec_function("date +%N")
    exec_file = user_cmds_folder + '/' + exec_file.strip().decode('utf-8')
    exec_function("touch " + exec_file)
    exec_function("chmod a+rw " + exec_file)
    return exec_file
 
def copy_execs(execs, exec_dir):
    exec_list = execs.split()
    final_execs = []
    for exec_file in exec_list:
        exec_function("cp " + exec_file + " " + exec_dir)
        final_execs.append(exec_dir + '/' + os.path.basename(exec_file))
    return final_execs

def get_log_file(prefix = ""):
  [output, err, ret] = exec_function("date +%N")
  if prefix != "":
      logfile = "/media/shared/ephemeral16/celerylogs/" + str(notify) + "/" + str(prefix) + "/" + output.strip().decode('utf-8')
  else:
      logfile = "/media/shared/ephemeral16/celerylogs/" + str(notify) + "/" + output.strip().decode('utf-8')
  return logfile

def write_exception( program, err, start, i, queue_name, traceback_info ):
   now = datetime.datetime.now()
   print('Program: ' + program)
   print('Curr time: ' + str(now))
   print('Started at: ' + str(start))
   print(traceback_info)
   print('Err: ' + str(err))
   print("-----------------------------")

def printId( id ):
    print("http://52.0.55.252:5555/tasks/" + id)
#    print "URL: http://54.210.198.184:5555/task/" + id

def execute(program, logfile, duration, branch=None):
 print("Program:", program.strip())
 start = datetime.datetime.now()
 err = Exception()
 for i in range(0, 1):
   try:
      #send task to celery
      result = ""
      if nowait:
            if branch == None:
                result = exec_job_nores.apply_async(args=[program, notify, logfile, nomail, duration], queue=queue_name, routing_key=queue_name, retry=True, retry_policy={'max_retries' : 3}, soft_time_limit = soft_time_limit_num, time_limit = time_limit_num)
            else:
                result = exec_branch.apply_async(args=[program, branch, notify, logfile, nomail, duration], queue=queue_name, routing_key=queue_name, retry=True, retry_policy={'max_retries' : 3}, soft_time_limit = soft_time_limit_num, time_limit = time_limit_num)
            printId(result.id)
            print("-----------------------------")
            return 0
      else:
            result = exec_job.apply_async(args=[program, notify, duration], queue=queue_name, routing_key=queue_name, retry=True, retry_policy={'max_retries' : 3}, soft_time_limit = soft_time_limit_num, time_limit = time_limit_num)
            printId(result.id) 

            #get result within given time out
            [output, err, ret_code] = result.get(timeout = time_out)
            print("Stdout: ")
            print(output)
            if err != None:
                    print("Stderr: ")
                    print(err)
            print("-----------------------------")

            return ret_code
   except Exception as err:
      print("Error! ") 
      write_exception( program, err, start, 0, queue_name, str(traceback.format_exc()) ) 
 #throw error in case there is exception
 raise err

final_execs = []
exec_dir = ''
if custom_execs:
    exec_dir = get_exec_dir()
    final_execs = copy_execs(execs, exec_dir)

err = Exception()
logfile = get_log_file()

def get_cmd():
    if not is_file:
        return prog
    else:
        comb_prog = ''
        with open(prog_file) as f:
            lines = f.readlines()
            for line in lines:
                program = line.strip()
                program = program.strip(';')
                if program != '':
                    comb_prog = comb_prog + program + '; '
        return comb_prog

is_branch = args.basetrade_branch != 'basetrade' or args.dvccode_branch != 'master' or args.dvctrade_branch != 'master'
if is_branch:
    cmd = get_cmd()
    branch = [args.basetrade_branch, args.dvccode_branch, args.dvctrade_branch]
    try:
        ret_code = execute(cmd, logfile, duration, branch)
        sys.exit(ret_code)
    except Exception as err:
        print(err)
    exit()

def check_msg_limit_exceeded(num_tasks):
    curr_tasks = get_curr_tasks()
    print("Tasks in queue: ", curr_tasks, " Tasks Submitted: ", num_tasks, " Max Limit: ", MAX_MSGS)
    return curr_tasks + num_tasks > MAX_MSGS


def run_tasks(tasks, cmnds=[], check_queue = True):
    if (not instant) and check_queue and check_msg_limit_exceeded(len(tasks)):
        file_content = '\n'.join(cmnds)
        file_path = get_scheduled_file_path()
        with open(file_path, 'w') as f:
            f.write(file_content + '\n')
        print("Currently Queue Full! Will be scheduled later. File:", file_path)
        return
    try:
        callback = group_clean_and_notify.s(prog_file, nomail).set(queue=queue_name, routing_key=queue_name, retry=True, retry_policy={'max_retries' : 3}, soft_time_limit = soft_time_limit_num, time_limit = time_limit_num)
        grouped = group(tasks)
        result=grouped.apply_async()
        chord_val = chord(grouped,retry=True, retry_policy={'max_retries' : 3}, link=callback)
            #grouped.apply_async(retry=True, retry_policy={'max_retries' : 3}, link=callback))
    except Exception as err:
        print("Error!")
        write_exception( program, err, start, 0, queue_name, str(traceback.format_exc()) )
        sys.exit(1)
    result.save()    
    if not summary:
        for i in range(0, len(result)):
            print("Logfile: " + logfiles[i] + "\n")
            print("Program:", programs[i])
            printId(result[i].id)
            print("-----------------------------")
    print("Group ID: ", result.id)

if queue_name == 'slow':
    num_chunk = 15
elif queue_name == 'autoscalegroup':
    num_chunk = 10000
else:
    num_chunk = 1000

if not is_file:
    try:
        prog = prog.strip()
        if custom_execs:
            prog = subtitute_exec(final_execs, prog)
            prog = prog + '; rm -r ' + exec_dir
        print("Logfile: " + logfile + "\n")
        ret_code = execute(prog, logfile, duration)
        sys.exit(ret_code)
    except Exception as err:
        print(err)
else:
    tasks = []
    programs = []
    logfiles = []
    [output, err, ret] = exec_function("date +%N")
    prefix = output.strip().decode('utf-8')
    with open(prog_file) as f:
        lines = f.readlines()
        for line in lines:
          program = line.strip()
          if custom_execs:
              program = subtitute_exec(final_execs, program)
          programs.append(program)
          if not samelog:
            logfile = get_log_file(prefix)
          logfiles.append(logfile)

          tasks.append( exec_job_nores.s(program, notify, logfile, nomail, duration).set(queue=queue_name))
        #Check if whole can be inserted at once
        if (not instant) and queue_name != 'autoscalegroup' and check_msg_limit_exceeded(len(tasks)):
            file_content = '\n'.join(programs)
            file_path = get_scheduled_file_path()
            with open(file_path, 'w') as f:
                f.write(file_content + '\n')
            print("Currently Queue Full! Will be scheduled later. File:", file_path)
        else:
            chunks = [tasks[x:x+num_chunk] for x in range(0, len(tasks), num_chunk)]
            program_chunks = [programs[x:x+num_chunk] for x in range(0, len(programs), num_chunk)]
            for i in range(0, len(chunks)):
                run_tasks(chunks[i],program_chunks[i], queue_name != 'autoscalegroup')
