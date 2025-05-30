#!/usr/bin/env python
'''
Input:
Task ID or group ID to track

Sample Execution:
python wait.py Task ID or group ID to track [Optional time bound before revoking the task and exit]
Example:
python wait.py -i jdahkjujb12332@45

This function polls on task/tasks status specified until they finishes or passed the maximum time specified by default it is 2000 seconds.
'''
from proj.workerProg import exec_job, exec_job_nores
from celery.result import AsyncResult
from celery.result import GroupResult
from datetime import datetime
from datetime import timedelta
import argparse
import sys
import MySQLdb
from proj.celery import app
import traceback
import time
from support import get_curr_tasks
from time import time

parser = argparse.ArgumentParser()
parser.add_argument("--id", "-i", help="Id of task")
parser.add_argument("--groupid", "-g", help="Group id of task")
parser.add_argument("--url", "-u", help="URL of task")
parser.add_argument("--limit", "-l", help="time to wait before we automatically revoke the task")
args = parser.parse_args()

#seconds to wait before revoking a job 

max_time_limit=2000

start_time = time()


def get_id(args):
    if args.id != None:
        return args.id
    elif args.url != None:
        url = args.url.strip()
        id = url.split('/')[-1]
        return id
    else:
        return None

def cancel_task(result):
     app.control.revoke(result.id, terminate=True)


def process_t(result):
    flag=0
    if result.state == "SUCCESS":
        return_data = result.get()
    elif result.state == "FAILURE" or result.state == "REVOKED":
        try:
            result.get()
        except:
            sys.exc_info()[1]
    elif result.state == "PENDING" or result.state == "STARTED" or result.state == "RECEIVED" or result.state == "RETRY":
        flag=1
    return flag    
   
def process_task(job_id):
    result = exec_job_nores.AsyncResult(job_id)
    flag=1;
    revoke=0
    while(flag):
        end_time = time()
        time_taken = end_time - start_time
        global max_time_limit 
        if int(time_taken) > int(max_time_limit):
            revoke=1
        if revoke:
            view_job_status.cancel_task(result)
            flag=0;
        else:
            flag=process_t(result);

def process_group(group_id):
    result = GroupResult.restore(group_id)
    flag=1;
    revoke=0
    global max_time_limit 
    while(flag):
        end_time = time()
        time_taken = end_time - start_time
        if int(time_taken) > int(max_time_limit):
            revoke=1
            if revoke:
                for task in result.children:
                    flag=0
                    cancel_task(task)
        else:
            flag=result.waiting();
       
job_id = get_id(args)

if args.limit != None:
  max_time_limit=args.limit
if job_id != None:
    process_task(job_id)
elif args.groupid != None:
    group_id = args.groupid
    process_group(group_id)
else:
    parser.print_help()

