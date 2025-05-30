
from proj.celery import app
from proj.email_lib import email_task, prog_output
from proj.get_email_id import get_email_id
import os
import sys
import subprocess
import socket
from celery.exceptions import SoftTimeLimitExceeded
import signal
#from exceptions import SystemExit
import time
from datetime import datetime
import fcntl
import os
import shutil
import json
from proj.support import exec_function


@app.task(name='workerProg.local_exec_func')
def local_exec_func(prog):
   process = subprocess.Popen(prog, stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
   (output, err) = process.communicate()
   ret = process.wait()
   return [output, err, ret]

@app.task(name='workerProg.exec_func')
def exec_func(prog):
    process = subprocess.Popen(prog, stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.STDOUT)
    (output, err) = process.communicate()
    ret = process.wait()
    if ret != 0:
        raise ValueError("Task '{0}' failed.\n Return Code: {1}\n STDOUT:\n {2}\n STDERR:\n {3}\n".format(" ".join(prog), ret, output, err))
    return [output, err, ret]

@app.task(name='workerProg.exec_func_nores', ignore_result=True)
def exec_func_nores(prog, output_file):
    process = subprocess.Popen(prog, stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
    (output, err) = process.communicate()
    ret = process.wait()

    directory = os.path.dirname(output_file)
    if not (os.path.exists(directory) or directory == ''):
        os.makedirs(directory)

    logfile = open(output_file, 'w+')
    logfile.write(str(output))
    if err != None:
        logfile.write(str(err))
    logfile.close()
    if ret != 0:
        raise ValueError("Task '{0}' failed.\n Return Code: {1}\n STDOUT:\n {2}\n STDERR:\n {3}\n".format(prog, ret, output, err))

@app.task(name='workerProg.exec_job')
def exec_job(prog, user):
    output = ''
    try:
        process = subprocess.Popen(prog, stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True, preexec_fn=os.setsid)
        (output, err) = process.communicate()
        ret = process.wait()
    except SoftTimeLimitExceeded:
        os.killpg(os.getpgid(process.pid), signal.SIGKILL)
        ret = None
    output = "Hostname: " + socket.gethostname() + "\n" + output
    if ret == None:
        raise ValueError("Task '{0}' failed.\n Return Code: {1}\n STDOUT:\n {2}\n STDERR:\n {3}\n".format(prog, ret, output, 'TimeLimitExceeded!'))
    elif ret != 0:
        raise ValueError("Task '{0}' failed.\n Return Code: {1}\n STDOUT:\n {2}\n STDERR:\n {3}\n".format(prog, ret, output, err))
    return [output, err, ret]

def cleanup_and_notify(prog, user, output_file, nomail, ret, additional = ''):
    output = []
    if os.path.isfile(output_file):
        with open(output_file) as f:
            output = f.readlines()
    output = " ".join(output)
    output = output + additional
    if ret == 3000:
        output = "Time Limit Exceeded!\n" + str(output)
    elif ret == 2000:
        output = 'SystemExit exception. Possible cause: Revoke called on task \n' + str(output)
    if nomail == 0:
        user = get_email_id(user)
        email_task(prog, output, ret, user, socket.gethostname())
    if ret == 3000:
        raise ValueError("Task '{0}' failed.\n Return Code: {1}\n STDOUT:\n {2}\n STDERR:\n {3}\n".format(prog, ret, output, 'TimeLimitExceeded!'))
    elif ret == 2000:
        raise ValueError("Task '{0}' failed.\n Return Code: {1}\n STDOUT:\n {2}\n STDERR:\n {3}\n".format(prog, ret, output, 'SystemExit exception. Possible cause: Revoke called on task \n'))
    elif ret != 0:
        raise ValueError("Task '{0}' failed.\n Return Code: {1}\n STDOUT:\n {2}\n STDERR:\n {3}\n".format(prog, ret, output, ''))
    return [output, '', ret]


@app.task(name='workerProg.add_duration')
def add_duration(duration, duration_tag):
    data = {}
    json_file = '/mnt/sdf/logs/json_file'
    with open(json_file) as data_file:    
        data = json.load(data_file)
    if duration_tag in data:
        data[duration_tag][0] = str((float(data[duration_tag][0]) + duration))
        data[duration_tag][1] = str(int(data[duration_tag][1]) + 1)
    else:
        data[duration_tag] = [0, 0]
        data[duration_tag][0] = str(duration)
        data[duration_tag][1] = '1'
    with open(json_file, 'w') as outfile:
        json.dump(data, outfile)

@app.task(name='workerProg.exec_job_nores') #, ignore_result=True)
def exec_job_nores(prog, user, output_file, nomail, duration):
    directory = os.path.dirname(output_file)
    if not (os.path.exists(directory) or directory == ''):
        try:
            os.makedirs(directory)
        except OSError:
            pass
    output = ''
    err = ''
    ret = 0
    start = datetime.now()
    with open(output_file, "w") as outfile:
        process = subprocess.Popen(prog, stdout=outfile, stdin=subprocess.PIPE, stderr=outfile, shell=True, preexec_fn=os.setsid)
    try:
        ret = process.wait()
        if ret == 0 and duration != None:
            time_duration = (datetime.now() - start).seconds
            print(time_duration, duration)
            exec_function('~/dvccode/scripts/datainfra/celeryFiles/celeryClient/celeryScripts/add_duration.py ' + str(time_duration) + ' ' + str(duration))
        [output, err, ret] = cleanup_and_notify(prog, user, output_file, nomail, ret)
    except SoftTimeLimitExceeded:
        os.killpg(os.getpgid(process.pid), signal.SIGKILL)
        [output, err, ret] = cleanup_and_notify(prog, user, output_file, nomail, 3000)
    except SystemExit:
        os.killpg(os.getpgid(process.pid), signal.SIGKILL)
        [output, err, ret] = cleanup_and_notify(prog, user, output_file, nomail, 2000)
    time_duration = (datetime.now() - start).seconds
    return [prog, user, output_file, output, err, ret, time_duration]

def get_repo_path(task_id):
    dest = '/spare/local/celeryExecs/' + task_id
    return dest

def delete_repo_copy(path):
    shutil.rmtree(path)

@app.task(name='workerProg.exec_branch')
def exec_branch(prog, branch, user, output_file, nomail, duration):
    start = datetime.now()
    directory = os.path.dirname(output_file)
    proc_created = 0
    if not (os.path.exists(directory) or directory == ''):
        try:
            os.makedirs(directory)
        except OSError:
            pass
    output = ''
    err = ''
    ret = 0 
    cwd = None
    env = None
    [ exec_dir, err, ret ] = exec_function("date +%N")
    exec_dir = exec_dir.strip()
    new_repo_path = get_repo_path( exec_dir )
    try: 
        if os.path.exists(new_repo_path): 
            raise ValueError('Path exists! '+ new_repo_path)
        os.makedirs(new_repo_path)
        subprocess.check_call("git clone -b " + branch[0] + " git@github.com:cvquant/basetrade.git", shell=True, cwd=new_repo_path)
        new_repo_path = new_repo_path + "/basetrade"
        subprocess.check_call("git submodule init; git submodule update; cd dvccode; git checkout " + branch[1] + "; git pull; cd ../; cd dvctrade; git checkout " + branch[2] + "; git pull; cd ../", shell=True, cwd=new_repo_path)
        cwd = new_repo_path
        env = os.environ.copy()
        env['HOME'] = new_repo_path
        with open(output_file, "w") as outfile:
            ret = subprocess.check_call("bjam -j16 release", shell=True, cwd=new_repo_path, stdout=outfile, stdin=subprocess.PIPE, stderr=outfile)
            proc_created = 1
            process = subprocess.Popen(prog, stdout=outfile, stdin=subprocess.PIPE, stderr=outfile, shell=True, preexec_fn=os.setsid, cwd=new_repo_path)
            ret = process.wait()
            [output, err, ret] = cleanup_and_notify(prog, user, output_file, nomail, ret)
    except SoftTimeLimitExceeded:
        if proc_created:
            os.killpg(os.getpgid(process.pid), signal.SIGKILL)
        [output, err, ret] = cleanup_and_notify(prog, user, output_file, nomail, 3000)
    except SystemExit:
        if proc_created:
            os.killpg(os.getpgid(process.pid), signal.SIGKILL)
        [output, err, ret] = cleanup_and_notify(prog, user, output_file, nomail, 2000)
    except Exception as err:
        if proc_created:
            os.killpg(os.getpgid(process.pid), signal.SIGKILL)
        [output, err, ret] = cleanup_and_notify(prog, user, output_file, nomail, 1000, additional = str(err))
         
    delete_repo_copy(cwd)
    time_duration = (datetime.now() - start).seconds
    return [prog, user, output_file, output, err, ret, time_duration]

@app.task(name='workerProg.group_clean_and_notify')
def group_clean_and_notify( outputs, program, nomail ):
#    print program
    email_msg = ""
#    print outputs
    for output in outputs:
         print(output)
         email_msg = email_msg + prog_output( list(map(str, output )))
    user = outputs[0][1]
    [output, err, ret] = cleanup_and_notify(program, user, "", nomail, 0, additional=email_msg)
    return [ outputs ]
