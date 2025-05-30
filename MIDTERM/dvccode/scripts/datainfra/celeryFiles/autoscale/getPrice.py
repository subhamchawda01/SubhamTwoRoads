import datetime
import sys
import subprocess
import json
import time
import os.path
import os

def exec_func(prog):
    #print prog
    process = subprocess.Popen(prog, shell=True, stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.STDOUT)
    (output, err) = process.communicate()
    ret = process.wait()
    return output

def get_results(seconds, instance_type):
    now = datetime.datetime.utcfromtimestamp(time.time())
    start_time = now - datetime.timedelta(seconds=seconds)
    ans = exec_func('aws ec2 describe-spot-price-history --instance-types ' + instance_type + ' --start-time ' + start_time.isoformat() + ' --end-time ' + now.isoformat() + ' --filters Name=availability-zone,Values=us-east-1a Name=product-description,Values="Linux/UNIX (Amazon VPC)"')
    json_ans = json.loads(ans)
    res = []
    for line in json_ans[unicode('SpotPriceHistory')]:
        res.append(float(line[unicode('SpotPrice')].encode()))
    return res

#res = get_results(30, 'c3.8xlarge')
#print res
#print "Max: ", max(res)
#print "Avg: ", sum(res)/float(len(res))
