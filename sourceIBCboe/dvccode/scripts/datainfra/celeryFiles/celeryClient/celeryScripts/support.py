import sys
import subprocess
from pyrabbit.api import Client

def exec_function(prog):
    process = subprocess.Popen(prog, stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
    (output, err) = process.communicate()
    ret = process.wait()
    if ret != 0:
        print("Program: " + prog + "\nStdout: " + output.strip() + " ,Stderr: " + str(err))
        sys.exit(1)
    return [output, err, ret]


def get_curr_tasks():
    #[ output, err, ret ] = exec_function("ssh ec2-user@52.91.139.132 \"sudo /usr/sbin/rabbitmqctl list_queues -p vhostClient name messages | grep -w autoscalegroupmanual\" | awk -F' ' '{print $NF}'")
    #output = output.split()[-1].strip()
    #return int(output)
    queue_name = 'autoscalegroupmanual'
    rabbit_cli = Client('52.91.139.132:15672', 'test', 'test')
    queue_size = rabbit_cli.get_queue_depth('vhostClient', 'slow')
   # queue_size = queue['messages']
    return queue_size
