from run_bash import exec_bash
import json

def getKey():
    instance_id = exec_bash('ec2-metadata -i')
    instance_id = instance_id[13:-1]
    instance_info = exec_bash('aws ec2 describe-instances --instance-ids ' + instance_id.decode())
    instance_json = json.loads(instance_info.decode())
    infoarr = instance_json[str('Reservations')][0][str('Instances')][0][str('Tags')]
    for info in infoarr:
        if info[str('Key')] == str('aws:autoscaling:groupName'):
            #print info[unicode('Value')]
            return info[str('Value')].encode()
print(getKey())
