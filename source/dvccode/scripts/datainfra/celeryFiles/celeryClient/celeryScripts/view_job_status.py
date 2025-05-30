#!/usr/bin/env python
'''
Input:
Task ID or group ID to track and various other optional parameters

Sample Execution:
python view_job_status.py -i Task ID or -g group ID to track [Optional time bound before revoking the task and exit]
Example:
python view_job_status.py -i jdahkjujb12332@45

This function tells about task/group status in real time. It shows stdout, stderr and time taken.
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

parser = argparse.ArgumentParser()
parser.add_argument("--id", "-i", help="Id of task")
parser.add_argument("--groupid", "-g", help="Group id of task")
parser.add_argument("--url", "-u", help="URL of task")
parser.add_argument("--notify", "-n", help="Notify user provided to task")
parser.add_argument("--days", "-d", help="Fetches results for given days. Used only when -n option provided. Default = 3 days", default="3")
parser.add_argument("--limit", "-l", help="Total no of results to display. Default=25. For all result use -1", default=25)
parser.add_argument("--revoke", "-r", help="Revoke the task if 1 provided")
parser.add_argument("--summary", "-s", help="Print only summary of task. Used when group id provided")
parser.add_argument("--state", "-st", help="Get output of all ids of given state. E.g: SUCCESS/FAILURE")
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

def process_task(result):
  if (args.state != None and args.state == result.state) or args.state == None:
    print("ID: ", result.id)
    print("STATE: ", result.state)
    #if result.state == "RUNNING":
        #print "This task is currently RUNNING with status: {0}".format(result.result)
     
    if result.state == "SUCCESS":
        #print "This task ran SUCCESSFULLY with the following return data.\n"
        return_data = result.get()
        print("TASK: ", return_data[0])
        print("USER: ", return_data[1])
        print("LOGFILE: ", return_data[2])
        print("STDOUT:\n", return_data[3])
        print("STDERR:\n", return_data[4])
        print("RETURN CODE: ", return_data[5])
        print("TIME TAKEN: ", time.strftime('%H:%M:%S', time.gmtime(return_data[6])))
    elif result.state == "FAILURE":
        print("Stacktrace \n")
        try:
            result.get()
        except:
            print(sys.exc_info()[1])
    elif result.state == "PENDING":
        print("Queue Size: ", get_curr_tasks())
            
    print("-----------------------------")

def process_id(job_id, revoke):
    result = exec_job_nores.AsyncResult(job_id)
    if revoke:
        cancel_task(result)
    else:
        process_task(result)

def get_all_ids(user, num_days, limit):
    from_date = datetime.strftime(datetime.now()-timedelta(num_days), "%Y-%m-%d")
    cmd = ""
    if limit == -1:
        cmd = "select task_id from celery_taskmeta where result like '%" + user+"%' and date_done>='" + from_date + "' order by date_done desc"
    else:
        cmd = "select task_id from celery_taskmeta where result like '%" + user+"%' and date_done>='" + from_date + "' order by date_done desc limit " + str(limit)
    ids = []
    connection = MySQLdb.connect (host = "10.0.0.31", user = "dvcreader", passwd = "dvcwiki", db = "celeryDB")
    cursor = connection.cursor ()
    cursor.execute (cmd)
    data = cursor.fetchall()
    for row in data:
        ids.append(" ".join(map(str, row)))
        #print " ".join(map(str, row))
    cursor.close ()
    connection.close ()
    return ids


job_id = get_id(args)
revoke = (args.revoke != None)
summary = (args.summary != None)
if __name__ == "__main__":
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
        elif not summary:
            process_task(task)
 elif args.notify != None:
    ids = get_all_ids(args.notify, int(args.days), args.limit)
    for job_id in ids:
        process_id(job_id, revoke)
 else:
    parser.print_help()
#    print ids
