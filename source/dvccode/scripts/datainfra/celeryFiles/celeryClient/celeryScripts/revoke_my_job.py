#!/usr/bin/env python

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

parser = argparse.ArgumentParser()
parser.add_argument("--id", "-i", help="Id of task")
parser.add_argument("--groupid", "-g", help="Group id of task")
parser.add_argument("--url", "-u", help="URL of task")

args = parser.parse_args()

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

def process_id(job_id, revoke):
    result = exec_job_nores.AsyncResult(job_id)
    if revoke:
        cancel_task(result)

job_id = get_id(args)
revoke = True

if job_id != None:
    process_id(job_id, revoke)
elif args.groupid != None:
    group_id = args.groupid
    result = app.GroupResult.restore(group_id)
    print("TOTAL: ", len(result.children))
    print("COMPLETED: ", result.completed_count())
    print("-----------------------------")
    for task in result.children:
        if revoke:
            cancel_task(task)
else:
    parser.print_help()
