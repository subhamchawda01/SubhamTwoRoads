import subprocess
import json

def instance_count(groupname):
   res = subprocess.Popen("aws autoscaling describe-auto-scaling-groups --auto-scaling-group-name " + groupname, shell = True, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
   [output, err] = res.communicate()
   jobj = json.loads(output)
   arr = jobj[u'AutoScalingGroups'][0][u'Instances']
   count = 0
   for instance in arr:
      if instance[u'LifecycleState'] == u'InService':
         count = count + 1
   return count

print instance_count('autoscalegroup')
