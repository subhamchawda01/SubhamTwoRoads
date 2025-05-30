import subprocess
import json
'''
def exec_func(prog):
    process = subprocess.Popen(prog, stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.STDOUT, shell = True)
    (output, err) = process.communicate()
    ret = process.wait()
    return [output, err, ret]

[output, err, ret] = exec_func("aws autoscaling describe-auto-scaling-instances --instance-ids `ec2-metadata -i | awk '{print $2}'`")

json_out = json.loads(output)
state = json_out[u'AutoScalingInstances'][0][u'LifecycleState']
if state == unicode('Terminating:Wait'):
    exec_func("ps aux | grep celery | awk '{print $2}' | xargs kill -SIGINT")
'''
